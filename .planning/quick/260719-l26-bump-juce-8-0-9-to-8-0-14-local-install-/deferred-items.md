# Deferred Items — quick task 260719-l26 (JUCE 8.0.9 → 8.0.14 bump)

## DEF-L26-01: O-TextureForge fails to compile — umappp ↔ irlba transitive-dependency skew (JUCE-independent, out of scope)

**Discovered during:** Task 2 (fresh full-suite build against patched JUCE 8.0.14)

**Status:** Deferred — out of scope for the JUCE bump. Do NOT fix as part of 260719-l26.

**Symptom (single failing target — blocks both O-TextureForge_VST3 and O-TextureForge_AU via the shared object):**

```
FAILED: plugins/O-TextureForge/CMakeFiles/OuariconTextureForge.dir/Source/dsp/UMAPProjection.cpp.o
.../build/_deps/umappp-src/include/umappp/spectral_init.hpp:123:17:
  error: no member named 'converged' in
  'irlba::Results<Eigen::Matrix<double, -1, -1>, Eigen::Matrix<double, -1, 1>>'
    123 |     if (!actual.converged) {
```

**Root cause:** `plugins/O-TextureForge/CMakeLists.txt` pins `umappp` to `GIT_TAG v3.2.0`, but
umappp's own transitive dependency `irlba` is fetched by umappp's internal CMake (not pinned by
O-TextureForge). The resolved irlba dropped the `Results::converged` member that umappp v3.2.0's
`spectral_init.hpp` still references — a version skew between umappp v3.2.0 and the irlba it pulls.

**Why it is unrelated to the JUCE 8.0.9 → 8.0.14 bump:** `UMAPProjection.cpp` → `umappp.hpp` →
`irlba/*.hpp` → `Eigen/*` includes **no JUCE headers whatsoever**. The failure is a pure
third-party C++ template API mismatch. The JUCE version has no bearing on this translation unit.
It surfaced only because Task 2's mandated fresh `rm -rf build` wiped the previously-cached
(compatible) irlba checkout, forcing a fresh FetchContent resolution to the drifted irlba.

**Scope decision:** Per the executor scope boundary ("only auto-fix issues DIRECTLY caused by the
current task's changes; log unrelated failures and do NOT fix them"). Re-pinning irlba is a
separate O-TextureForge dependency-hardening task that risks the plugin's load-bearing UMAP grain
selection — not appropriate to bundle into a JUCE version bump.

**Suggested fix (future task):** Pin irlba to a umappp-v3.2.0-compatible tag in
`plugins/O-TextureForge/CMakeLists.txt` (add an explicit `FetchContent_Declare(irlba GIT_TAG <compatible>)`
before `FetchContent_MakeAvailable(umappp)`, or bump umappp to a release whose bundled irlba
still provides `Results::converged`), then rebuild + regression-test O-TextureForge's UMAP path.

**Impact on this task:** 36 of 37 plugins build cleanly (VST3 + AU) against patched JUCE 8.0.14
with ZERO JUCE fallout. O-TextureForge is the sole exception and is JUCE-independent.
