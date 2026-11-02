# Quick Task 260719-qxy: Bump O-TextureForge header-only deps — Research

**Researched:** 2026-07-19
**Domain:** CMake FetchContent dependency pinning (nanoflann + umappp) for a JUCE granular plugin
**Confidence:** HIGH

## Summary

Two header-only deps in `plugins/O-TextureForge/CMakeLists.txt` are being bumped: **umappp v3.2.0 → v3.3.2** and **nanoflann v1.6.2 → highest safe tag**.

- **nanoflann:** Our usage surface (static `KDTreeSingleIndexAdaptor` + `KNNResultSet` + `findNeighbors` + `SearchParameters`) is stable across every release from v1.7.0 through 1.10.1. No breaking change touches us. **PIN `1.10.1`** (exact tag string, no `v` prefix). Fallback `v1.7.1` is not needed.
- **umappp:** The task brief's premise ("Options threading refactor doesn't touch our usage") is **WRONG**. `UMAPProjection.cpp:58` uses `opt.parallel_optimization = false;` — that exact field was **removed at umappp v3.3.0** (converted to `num_threads_optimize`). Bumping to v3.3.2 **will not compile** until line 58 is migrated to `opt.num_threads_optimize = 1;`. This is the load-bearing finding of this research.
- **Fresh-build / irlba drift:** umappp v3.3.2 **still uses `GIT_TAG master`** for its transitive irlba and knncolle (irlba repo moved `LTLA/CppIrlba` → `libscran/irlba`). The bump likely *resolves the current* `no member 'converged'` breakage (v3.3.x code tracks current irlba master), but does **not structurally guarantee** future fresh builds — irlba remains an unpinned moving ref.

**Primary recommendation:** Write `GIT_TAG 1.10.1` (nanoflann) and `GIT_TAG v3.3.2` (umappp), **AND** edit `UMAPProjection.cpp:58` `parallel_optimization = false` → `num_threads_optimize = 1`. Force a clean re-fetch of the umappp/irlba/knncolle deps to genuinely exercise the fresh-build path, then run the standard install + auval + pluginval battery.

## Current Pins (exact syntax)

`plugins/O-TextureForge/CMakeLists.txt`:
- Lines 7–11: `FetchContent_Declare(nanoflann ... GIT_TAG v1.6.2)`  → change to `GIT_TAG 1.10.1`
- Lines 17–21: `FetchContent_Declare(umappp ... GIT_TAG v3.2.0)`   → change to `GIT_TAG v3.3.2`

Linked as `nanoflann::nanoflann` and `libscran::umappp` (CMakeLists lines 70–71). `[VERIFIED: file read]`

## Tag Verification (git ls-remote)

**nanoflann** — the v-prefix is inconsistent; the highest tag is unprefixed: `[VERIFIED: git ls-remote]`

| Version | Exact tag string | Notes |
|---------|-----------------|-------|
| 1.7.0 | `v1.7.0` | prefixed |
| 1.7.1 | `v1.7.1` | prefixed (fallback) |
| 1.8.0 | `v1.8.0` **and** `1.8.0` | both exist |
| 1.9.0 | `v1.9.0` | prefixed only |
| 1.10.0 | `v1.10.0` | prefixed only |
| **1.10.1** | **`1.10.1`** | **unprefixed only — no `v1.10.1` exists** |

→ Highest safe tag GIT_TAG string is exactly **`1.10.1`** (no `v`). This matches the task's v-prefix-drop note.

**umappp** — `v3.3.2` exists exactly as spelled (also v3.2.1, v3.3.0, v3.3.1). `[VERIFIED: git ls-remote]`

## Our nanoflann Usage Inventory

Grep of `plugins/O-TextureForge/Source/` — every nanoflann symbol we touch: `[VERIFIED: grep + file read]`

| Symbol | File:line | Notes |
|--------|-----------|-------|
| `#include <nanoflann.hpp>` | KDTreeSearch.h:13 | single-header |
| `L2_Simple_Adaptor<float, GrainDescriptorAdaptor>` | KDTreeSearch.h:44 | default IndexType (relevant to 1.8.0 change, see below) |
| `KDTreeSingleIndexAdaptor<Distance, GrainDescriptorAdaptor, 19, uint32_t>` | KDTreeSearch.h:45–49 | 4 template params; **IndexType explicitly = uint32_t** |
| `KDTreeSingleIndexAdaptorParams(10)` | KDTreeSearch.cpp:18 | max-leaf ctor |
| `index->buildIndex()` | KDTreeSearch.cpp:21 | |
| `KNNResultSet<float, uint32_t>` + `.init()` + `.size()` | KDTreeSearch.cpp:32–38 | explicit IndexType = uint32_t |
| `SearchParameters` | KDTreeSearch.cpp:35 | **already the modern name** (post SearchParams→SearchParameters rename) |
| `index->findNeighbors(resultSet, queryPoint, searchParams)` | KDTreeSearch.cpp:36 | |
| Adaptor methods `kdtree_get_point_count` / `kdtree_get_pt` / `kdtree_get_bbox` | KDTreeSearch.h:25,27,32 | static dataset adaptor |

**NOT used** (confirmed by grep): `knnSearch`, `radiusSearch`, `saveIndex`/`loadIndex`, `KDTreeSingleIndexDynamicAdaptor`, `ResultItem`, `worstDist`. We use the static index + manual `KNNResultSet` path only. This is the most stable corner of the API.

## nanoflann Changelog Scan v1.7.0 → 1.10.1 (vs our usage)

Source: GitHub releases API (`gh api repos/jlblancoc/nanoflann/releases`). `[CITED: github.com/jlblancoc/nanoflann/releases]`

| Version | API-relevant change | Impact on us |
|---------|--------------------|--------------|
| **1.7.0** | `ResultSets::worstDist()` return-value clarification (returns actual worst dist only when set full) | None — we never call `worstDist` directly |
| **1.7.1** | `worstDist()` negative-index static-analysis fix | None — internal safety |
| **1.8.0** | PR#266: **default `IndexType` in metric adaptor classes changed to `size_t`**; PR#272 new `findWithinBox`; PR#263 no longer writes to stderr | **None** — we explicitly pass `uint32_t` to the KDTree and `KNNResultSet`. `L2_Simple_Adaptor` uses the new `size_t` default only for its internal `b_idx` param; the KDTree's `uint32_t` index converts to it losslessly. Compiles clean. |
| **1.9.0** | L1/L2 distance functor optimizations (PR#277/#279); dynamic-index `sorted` fix (PR#276) | None to API. Distance micro-opts *could* produce bit-identical-or-marginally-different FP → theoretical NN tie reordering only. Not a correctness/API break. Dynamic-index fix irrelevant (we use static). |
| **1.10.0** | Many fixes to `KDTreeSingleIndexDynamicAdaptor` (removePoint/addPoints), `saveIndex`/`loadIndex` header format, `middleSplit_` heap-alloc removal, **widened `[[nodiscard]]`** (PR#299), remove dead `worst_dist` branch in L1/L2 `evalMetric` (PR#291) | None — all touched surfaces are ones we don't use (dynamic adaptor, save/load). Widened `[[nodiscard]]`: only bites if `-Werror` + a discarded return; our calls consume their returns (`resultSet.size()`) — safe. |
| **1.10.1** | PR#303: refactor unit **tests** into smaller files | **Zero** library/API impact — test-only |

**Latest released version: 1.10.1.** No SearchParams→SearchParameters rename in this range (that happened ~v1.5; we're already on `SearchParameters`). No C++ standard requirement bump (nanoflann still ≥C++11; we build C++17/20 under JUCE). No header reorg affecting the single `nanoflann.hpp` include.

**Verdict: changelog scan is CLEAN for our usage across the entire 1.7.0–1.10.1 range.**

## PIN RECOMMENDATION

| Dep | Write GIT_TAG | Rationale |
|-----|--------------|-----------|
| **nanoflann** | **`1.10.1`** (no `v`) | Highest tag exists as unprefixed `1.10.1`; scan clean for our static KNN usage. Fallback `v1.7.1` unnecessary. |
| **umappp** | **`v3.3.2`** (with `v`) | Confirmed exact tag; drop-in for our `initialize`/`Options`/`run` surface **after the one required source edit** (below). |

## REQUIRED SOURCE EDIT (umappp v3.3.0 breaking change)

umappp release notes v3.3.0: *"Converted `Options::parallel_optimization` to `Options::num_threads_optimize`"* and *"Added `Options::num_threads_spectral`"*. `[CITED: github.com/libscran/umappp/releases v3.3.0]`

Our `UMAPProjection.cpp:58` currently:
```cpp
opt.parallel_optimization = false;   // ← field REMOVED at umappp v3.3.0 → won't compile
```
Migrate to (serial optimization == 1 thread):
```cpp
opt.num_threads_optimize = 1;
```
Our other Options (`num_neighbors`, `min_dist`, `num_threads`, `initialize_method`) are unchanged and still valid. Because we use `InitializeMethod::SPECTRAL`, optionally also set `opt.num_threads_spectral = 1;` for run-to-run determinism (irlba FP results vary with thread count) — recommended but not required. Our `umappp::initialize<int,double>(...)`, `status.num_epochs()`, and `status.run(...)` calls (lines 62–93) are unchanged across v3.2.0→v3.3.2. `[VERIFIED: file read + release notes]`

## umappp v3.2.0 → v3.3.2 Release Scan

`[CITED: github.com/libscran/umappp/releases]`

- **v3.2.1:** knncolle/subpar/sanisizer bumps; `parallel_optimization` fixed to actually be a `bool`.
- **v3.3.0:** latest irlba (perf); **`parallel_optimization` → `num_threads_optimize`** (the break above); added `num_threads_spectral`.
- **v3.3.1:** bumped required irlba version in `find_package()`.
- **v3.3.2:** minor `find_dependency()` fix for downstream `find_package(libscran_umappp)`.

Our knncolle usage (`EuclideanDistance<double,double>`, `VptreeBuilder<int,double,double>`, UMAPProjection.cpp:62–63) is on the stable knncolle 3.x surface and is unaffected by the umappp-side bundled-knncolle bump.

## Fresh-Build / irlba Drift — CRITICAL for executor

Known issue (memory `critical-otextureforge-umappp-irlba-drift`, DEF-L26-01): O-TextureForge fails **any fresh build** with `no member named 'converged' in irlba::Results`. Root cause confirmed by reading the fetched umappp v3.2.0 `extern/CMakeLists.txt`: irlba was pinned to **`GIT_TAG master`** (a moving ref) from `LTLA/CppIrlba`, which drifted upstream. `[VERIFIED: file read of build/_deps/umappp-src/extern/CMakeLists.txt]`

**umappp v3.3.2 `extern/CMakeLists.txt` (fetched from upstream):** `[VERIFIED: raw.githubusercontent v3.3.2]`
```cmake
FetchContent_Declare(knncolle ... GIT_TAG master # ^3.1.0)
FetchContent_Declare(irlba GIT_REPOSITORY https://github.com/libscran/irlba GIT_TAG master # ^3.1.0)
FetchContent_Declare(eigen  ... GIT_TAG 5.0.0)
```
Key facts:
- irlba is **still `GIT_TAG master`** (unpinned) — but the **repo URL changed** `LTLA/CppIrlba` → `libscran/irlba`.
- Because the repo URL differs, FetchContent will **re-fetch irlba** on reconfigure even in the existing `build/` tree (stamp invalidated by the changed declare).
- umappp v3.3.x code (spectral_init) is written against current `libscran/irlba` master, so a fresh build **today** should compile — this bump most likely **clears** the DEF-L26-01 breakage.
- It does **not** structurally fix the drift: irlba is still a moving master and can drift again in future. A durable fix (out of this task's scope) would add explicit `FetchContent_Declare(irlba ... GIT_TAG <tag>)` / knncolle overrides *before* `FetchContent_MakeAvailable(umappp)`. Flag as follow-up.

**Executor guidance:**
- The existing `build/` dir (configured 2026-07-19 18:48, deps cached at v3.2.0-era) will re-fetch nanoflann (tag change v1.6.2→1.10.1) and umappp (v3.2.0→v3.3.2) automatically on reconfigure.
- To *genuinely* validate the fresh-build fix (stale trees HIDE the drift), force-clean the affected dep caches before rebuilding:
  ```bash
  rm -rf build/_deps/umappp-* build/_deps/irlba-* build/_deps/knncolle-* build/_deps/nanoflann-* build/_deps/eigen-*
  ```
  then reconfigure + build the O-TextureForge targets. If configure re-fetches and the compile is clean, the drift is resolved.
- If the reconfigure errors on a stale FetchContent stamp, escalate to a full `rm -rf build` fresh configure (this is exactly the path the memory says currently fails at v3.2.0 — it should now PASS at v3.3.2).
- UMAP grain-selection is **load-bearing** (see O-TextureForge drone/scatter bug history in MEMORY.md) — after building, verify grain selection still behaves in-DAW, not just that it compiles.

## Build / Verify Sequence (from CLAUDE.md + repo scripts)

**Build targets (macOS):**
```bash
ninja OuariconTextureForge_VST3 OuariconTextureForge_AU
```
Note: the juce_add_plugin **target is `OuariconTextureForge`**, not the folder name `O-TextureForge`, and PRODUCT_NAME is `O-TextureForge${OUARICON_DEV_SUFFIX}` (dev branding → `O-TextureForge-dev`). Prefer `./scripts/build-and-install.sh O-TextureForge` (resolves the target automatically and does the dual-variant cache sweep). `[VERIFIED: file read]`

**Mandatory cache-clear + dual-variant sweep before install** (CLAUDE.md — sweep BOTH `O-TextureForge.{vst3,component}` and `O-TextureForge-dev.{vst3,component}`):
```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
# remove both variants of VST3 + AU before copying fresh
```

**Verify:**
- `auval -a | grep -i textureforge` (AU registered)
- pluginval (repo standard, via `scripts/verify-suite-battery.sh`): `[VERIFIED: file read]`
  ```
  pluginval --strictness-level 8 --skip-gui-tests --timeout-ms <ms> --output-dir <logdir> --validate <installed .vst3>
  ```
- Test grain selection in a DAW (UMAP path is load-bearing).

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | libscran/irlba master **today** is API-compatible with umappp v3.3.2's spectral_init (i.e. fresh build now compiles) | Fresh-Build | If drifted again, fresh build still fails `converged`-style — mitigated by forcing clean re-fetch during THIS task so it's tested now, not later. |
| A2 | nanoflann 1.8.0 metric-adaptor default IndexType change (uint32_t→size_t) compiles clean given our explicit uint32_t KDTree param | Changelog Scan | If a hard type mismatch surfaces, the build will fail loudly at compile — trivially caught, fallback `v1.7.1` available. |

## Sources

**Primary (HIGH):**
- `plugins/O-TextureForge/CMakeLists.txt`, `Source/dsp/KDTreeSearch.{h,cpp}`, `Source/dsp/UMAPProjection.cpp` — file reads
- `git ls-remote --tags` for both repos — exact tag strings
- `build/_deps/umappp-src/extern/CMakeLists.txt` (v3.2.0) + `raw.githubusercontent.com/libscran/umappp/v3.3.2/extern/CMakeLists.txt` — irlba pin comparison
- `scripts/verify-suite-battery.sh` — pluginval invocation

**Secondary (MEDIUM):**
- `gh api repos/jlblancoc/nanoflann/releases` — nanoflann changelog v1.7.0–1.10.1
- `gh api repos/libscran/umappp/releases` — umappp v3.2.0–v3.3.2 notes
- MEMORY.md `critical_otextureforge_umappp_irlba_drift` — DEF-L26-01 context
