---
phase: 260719-qxy-bump-o-textureforge-header-only-deps-uma
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - plugins/O-TextureForge/CMakeLists.txt
  - plugins/O-TextureForge/Source/dsp/UMAPProjection.cpp
  - scripts/verify-suite-battery.sh
autonomous: false
requirements: [QXY-DEP-BUMP]
tags: [cmake, fetchcontent, umappp, nanoflann, o-textureforge, dependency-bump]

must_haves:
  truths:
    - "O-TextureForge builds fresh (umappp/irlba/knncolle/nanoflann/eigen re-fetched) with the new pins, clearing DEF-L26-01"
    - "pluginval --strictness-level 8 --skip-gui-tests passes on the installed O-TextureForge VST3"
    - "auval (verify-au-link.sh) passes for O-TextureForge"
    - "UMAP scatter + KD-tree grain-query behavior is unchanged vs the pre-bump build"
  artifacts:
    - plugins/O-TextureForge/CMakeLists.txt
    - plugins/O-TextureForge/Source/dsp/UMAPProjection.cpp
  key_links:
    - "umappp v3.3.2 bump REQUIRES the UMAPProjection.cpp num_threads_optimize migration in the SAME commit — the bump does not compile otherwise (parallel_optimization field removed at v3.3.0)"
    - "The force-clean dep re-fetch is what actually exercises the fresh-build path; a stale build/_deps tree HIDES the irlba drift (DEF-L26-01)"
    - "verify-suite-battery.sh KNOWN-FAIL guard for O-TextureForge must be removed once green, or the suite battery keeps skipping it forever"
---

<objective>
Bump O-TextureForge's two header-only FetchContent deps — nanoflann v1.6.2 → 1.10.1 and umappp v3.2.0 → v3.3.2 — migrate the one breaking umappp Options field, force a clean dep re-fetch to genuinely exercise the fresh-build path (clearing DEF-L26-01), and re-run the build/auval/pluginval battery for O-TextureForge alone.

Purpose: umappp v3.3.0 removed `Options::parallel_optimization` (converted to `num_threads_optimize`), so the bump will not compile until UMAPProjection.cpp is migrated. nanoflann's highest tag is the unprefixed `1.10.1` (no `v1.10.1` exists) and its changelog scan is clean for our static KNN usage. A stale `build/_deps` tree masks the known umappp↔irlba transitive drift, so the fresh re-fetch is mandatory to prove DEF-L26-01 is resolved.
Output: two dependency pins bumped, one source field migrated, a clean fresh build, and a passing auval + strictness-8 pluginval gate on O-TextureForge.
</objective>

<execution_context>
@$HOME/.claude/gsd-core/workflows/execute-plan.md
@$HOME/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@./CLAUDE.md
@.planning/quick/260719-qxy-bump-o-textureforge-header-only-deps-uma/260719-qxy-RESEARCH.md
@plugins/O-TextureForge/CMakeLists.txt
@plugins/O-TextureForge/Source/dsp/UMAPProjection.cpp
</context>

<tasks>

<task type="auto">
  <name>Task 1: Bump pins + migrate umappp Options field</name>
  <files>plugins/O-TextureForge/CMakeLists.txt, plugins/O-TextureForge/Source/dsp/UMAPProjection.cpp</files>
  <action>
Make three edits, all part of one atomic dependency bump:

1. CMakeLists.txt line 10: change the nanoflann pin `GIT_TAG v1.6.2` to `GIT_TAG 1.10.1`. Per RESEARCH.md, the highest nanoflann tag exists ONLY as the unprefixed string `1.10.1` — there is no `v1.10.1` tag. Do not add a `v` prefix here.

2. CMakeLists.txt line 20: change the umappp pin `GIT_TAG v3.2.0` to `GIT_TAG v3.3.2` (this tag keeps its `v` prefix — confirmed exact via git ls-remote).

3. UMAPProjection.cpp line 58: replace the removed `opt.parallel_optimization = false;` field with `opt.num_threads_optimize = 1;` (serial optimization == 1 thread). This field was removed at umappp v3.3.0; the bump will not compile without this migration. Immediately after it, also add `opt.num_threads_spectral = 1;` for run-to-run determinism, since our SPECTRAL init path runs irlba whose FP results vary with thread count (recommended by RESEARCH.md). Replace the line outright — do NOT leave the old field name behind in a comment. Leave every other Options field (`num_neighbors`, `min_dist`, `num_threads`, `initialize_method`) exactly as-is; they remain valid across v3.2.0 → v3.3.2.
  </action>
  <verify>
    <automated>grep -q 'GIT_TAG 1.10.1' plugins/O-TextureForge/CMakeLists.txt && grep -q 'GIT_TAG v3.3.2' plugins/O-TextureForge/CMakeLists.txt && grep -q 'num_threads_optimize = 1' plugins/O-TextureForge/Source/dsp/UMAPProjection.cpp && echo PINS_OK</automated>
  </verify>
  <done>nanoflann pinned to `1.10.1`, umappp pinned to `v3.3.2`, and UMAPProjection.cpp uses `num_threads_optimize` (+ optional `num_threads_spectral`) instead of the removed `parallel_optimization` field. Old-field removal is confirmed by the Task 2 compile (umappp v3.3.2 has no such field → compile error if it lingers).</done>
</task>

<task type="auto">
  <name>Task 2: Force-clean re-fetch, build, install, and gate O-TextureForge</name>
  <files>scripts/verify-suite-battery.sh</files>
  <action>
Exercise the FRESH-build path (a stale dep tree hides the irlba drift) and run the full gate battery for O-TextureForge alone.

1. Force-clean the affected dep caches so FetchContent re-fetches at the new tags: `rm -rf build/_deps/umappp-* build/_deps/irlba-* build/_deps/knncolle-* build/_deps/nanoflann-* build/_deps/eigen-*`. If a later reconfigure errors on a stale FetchContent stamp, escalate to `rm -rf build` and a full fresh configure — RESEARCH.md notes this is exactly the path that failed at v3.2.0 and is expected to PASS at v3.3.2.

2. Build + install via `./scripts/build-and-install.sh O-TextureForge` (resolves the `OuariconTextureForge` target automatically, clears the AU cache, and does the dual-variant `O-TextureForge` / `O-TextureForge-dev` sweep per CLAUDE.md). Confirm the fresh umappp/irlba/eigen compile is clean — no `no member named 'converged' in irlba::Results` (the DEF-L26-01 signature).

3. auval gate: `bash scripts/verify-au-link.sh O-TextureForge`.

4. pluginval gate on the freshly installed VST3 (dev branding → `O-TextureForge-dev.vst3`): run `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 8 --skip-gui-tests --timeout-ms 60000 --validate "$HOME/Library/Audio/Plug-Ins/VST3/O-TextureForge-dev.vst3"`.

5. ONLY after the build, auval, and pluginval all pass: remove the O-TextureForge KNOWN-FAIL early-return guard in `scripts/verify-suite-battery.sh` (the `if [ "$plugin" = "O-TextureForge" ]` block at ~lines 129-135 that writes KNOWN-FAIL and returns 0). Deleting it lets the suite battery build+gate O-TextureForge like every other plugin now that DEF-L26-01 is cleared. Do NOT touch this guard if any gate above fails — leave the block in place and report the failure instead.

Note the umappp irlba pin is still `GIT_TAG master` (unpinned transitive) — flag in the summary as a durable-fix follow-up, but it is out of scope for this bump.
  </action>
  <verify>
    <automated>/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 8 --skip-gui-tests --timeout-ms 60000 --validate "$HOME/Library/Audio/Plug-Ins/VST3/O-TextureForge-dev.vst3" && echo PLUGINVAL_OK</automated>
  </verify>
  <done>O-TextureForge re-fetches deps fresh and compiles clean (DEF-L26-01 signature gone), auval passes, pluginval strictness-8 passes on the installed VST3, and the verify-suite-battery.sh KNOWN-FAIL guard is removed (only if all three gates passed).</done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <what-built>Bumped nanoflann → 1.10.1 and umappp → v3.3.2 (with the `num_threads_optimize` migration), fresh-rebuilt and installed O-TextureForge, auval + pluginval strictness-8 green.</what-built>
  <how-to-verify>
UMAP grain-selection is load-bearing (see the O-TextureForge drone/scatter bug history in MEMORY.md — scatterX/Y and UMAP/PCA position bias). Compile-clean is not sufficient; confirm behavior:
  1. Open O-TextureForge-dev in a DAW (or Standalone).
  2. Load a corpus and let the UMAP projection compute — confirm the 2D scatter map renders and points spread as before (not collapsed / degenerate).
  3. Move scatterX / scatterY and the position controls — confirm grain selection audibly follows the UMAP scatter position (the drone/scatter regression class).
  4. Confirm KD-tree nearest-grain querying still returns sensible neighbors (no silence, no stuck single-grain drone).
Expected: scatter map + grain query behave identically to the pre-bump build.
  </how-to-verify>
  <resume-signal>Type "approved" or describe any scatter/grain-query regressions.</resume-signal>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| upstream git → build | FetchContent pulls third-party source at the pinned git tags at configure time (untrusted upstream code compiled into the plugin) |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-QXY-01 | Tampering | nanoflann/umappp pinned git tags | low | mitigate | Pin exact tags (`1.10.1`, `v3.3.2`) verified via git ls-remote in RESEARCH.md; header-only, no install-time script execution |
| T-QXY-02 | Tampering | umappp transitive irlba `GIT_TAG master` | medium | accept | irlba/knncolle remain unpinned moving refs upstream; force-clean re-fetch validates the current ref compiles now. Durable pin is a flagged follow-up, out of this bump's scope |
</threat_model>

<verification>
- Task 1 greps confirm both pins and the field migration.
- Task 2 fresh build compiles clean (DEF-L26-01 `converged` signature absent), auval passes, pluginval strictness-8 passes on the installed VST3.
- Task 3 human DAW check confirms UMAP scatter + KD-tree grain-query behavior is unchanged.
</verification>

<success_criteria>
- nanoflann pinned `1.10.1`, umappp pinned `v3.3.2`, UMAPProjection.cpp migrated to `num_threads_optimize`.
- Force-clean re-fetch produces a clean fresh build (DEF-L26-01 cleared).
- auval + pluginval --strictness-level 8 --skip-gui-tests both PASS on O-TextureForge.
- verify-suite-battery.sh no longer short-circuits O-TextureForge as KNOWN-FAIL.
- Human confirms UMAP scatter + grain-query behavior unchanged in a DAW.
</success_criteria>

<output>
Create `.planning/quick/260719-qxy-bump-o-textureforge-header-only-deps-uma/260719-qxy-SUMMARY.md` when done. In it, flag the umappp transitive irlba `GIT_TAG master` drift as a durable-fix follow-up (explicit irlba/knncolle FetchContent_Declare before MakeAvailable(umappp)), and note whether the KNOWN-FAIL guard was removed.
</output>
