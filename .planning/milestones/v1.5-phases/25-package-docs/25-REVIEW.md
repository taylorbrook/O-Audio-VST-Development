---
phase: 25-package-docs
status: issues_found
depth: standard
review_date: 2026-04-27T18:14:17Z
files_reviewed: 12
files_reviewed_list:
  - modules/tuning/note-expression/resources/library/Ouaricon-VST3-NoteExpression.doricolib
  - modules/tuning/note-expression/resources/README-microtonal-suite.txt
  - modules/tuning/note-expression/install-microtonal-suite.cmake.in
  - modules/tuning/note-expression/module.cmake
  - modules/tuning/note-expression/README.md
  - modules/cmake/OuariconModules.cmake
  - .claude/skills/plugin-packaging/references/pkg-creation.md
  - .claude/skills/plugin-packaging/assets/inno-template.iss
  - .claude/skills/plugin-packaging/references/inno-setup-creation.md
  - .planning/phases/25-package-docs/25-02-PREFLIGHT-AUDIT.md
  - .planning/phases/25-package-docs/25-02-VALIDATION-MATRIX.md
  - research/microtonal-dorico-integration.md
findings: { blocker: 1, high: 2, medium: 3, low: 2, info: 1 }
---

# Phase 25 (package-docs) — Code Review

**Reviewed:** 2026-04-27
**Depth:** standard
**Files Reviewed:** 12
**Status:** issues_found

## Summary

Phase 25 ships the Microtonal Suite Dorico distribution package via three plans: a canonical `.doricolib` plus a CMake `install(SCRIPT)` rule (Plan 25-01), shared PKG/Inno installer-template extensions covering all 8 cohort plugins (Plan 25-02), and an internal-developer technical reference (Plan 25-03).

Asset-side correctness is solid:

- The `.doricolib` is well-formed XML, has exactly 48 `<kScoreLibrary>` direct children, exactly one occurrence each of `kVST3NoteExpression` and `xmap.ouaricon.vst3_note_expression`, and is 6,431 B as claimed.
- No banned Path A residue (`dorico_pt`, `tar cf`, `Default Library Additions`, `DefaultLibraryAdditions`, `ouaricon_extract_vst3_cids`, `DORICO_PT_STAGE`, `ouaricon_microtonal_suite_pt`) appears in any source/installer file in scope. (One historical reference to `ouaricon_extract_vst3_cids` exists inside `research/microtonal-dorico-integration.md` line 481, intentionally documenting that the helper is dead code — narrative, not Path A residue.)
- The macOS PKG postinstall correctly handles `ACTUAL_USER` discovery and chowns the suite directory back to the user, avoiding a root-owned-asset trap.
- CMake `configure_file(... @ONLY)` substitutes only documented variables; the install-time CMake `file(COPY)` calls take only build-time-derived paths.

However, the Windows installer path has a load-bearing privilege/scope defect that breaks D-07 ("single-write to `%APPDATA%\Ouaricon\Microtonal Suite\`") on any system where the user clicking the installer is not the same Windows account that approves the UAC elevation prompt. Two additional issues degrade robustness, and three style issues were noted. Details below.

## BLOCKER

### BL-01: Inno Setup `{userappdata}` resolves to admin's profile under `PrivilegesRequired=admin`, not the launching user's profile

**File:** `.claude/skills/plugin-packaging/assets/inno-template.iss:39, 52-53, 81`
**Severity:** BLOCKER
**Issue:**
The template sets `PrivilegesRequired=admin` (line 39), then writes the Microtonal Suite payload to `{userappdata}\Ouaricon\Microtonal Suite` on lines 52-53 and logs the same path on line 81. Per Inno Setup documentation, when `PrivilegesRequired=admin` is set, the installer process runs under the credentials of whoever satisfies the UAC elevation prompt. Inno Setup's `{userappdata}` constant resolves to the **elevated process's** `%APPDATA%`. If the user who launches the installer is a non-admin and elevates via a separate admin account (common on managed/enterprise machines, family-account Macs ported to Windows, MDM environments), the `.doricolib` lands in the **admin**'s roaming profile — not the user's. Dorico, running as the original (non-admin) user, will not find the asset at the documented location and the Library Manager Import step will fail to discover the file at the path the README and DOCS-02 advertise.

This breaks D-07 ("single-write to platform-specific Ouaricon shared path") on every multi-account Windows environment. The macOS PKG postinstall already handles this case correctly (it derives `ACTUAL_USER` via `stat -f '%Su' /dev/console` and chowns the suite dir). The Windows side has no equivalent.

The matrix-PASS verdict in `25-02-VALIDATION-MATRIX.md` does not exonerate this: the canary was run on the user's single-account Windows 11 dev box where the elevating account == the launching user, masking the divergence. The bug surfaces on multi-user/managed installs that were not in the canary.

Inno Setup's `{commonappdata}` is wrong direction (writes to `C:\ProgramData`, not user-specific). The correct fix is to use `{autoappdata}` (auto* constants resolve to user-scope when running un-elevated and to per-user-of-elevation when running elevated) plus `PrivilegesRequiredOverridesAllowed=dialog` so users can choose, or to refactor to use `GetUserNameEx` + manual path construction in `[Code]`. Cleanest is the documented Inno Setup pattern below.

**Fix:**
```iss
; Replace line 39:
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog

; Replace lines 52-53 (and the Log() at line 81) — use {autoappdata} so the
; constant follows the actual process token and matches the user-side
; expectation in DOCS-02 / README-microtonal-suite.txt.
Source: "{{MICROTONAL_SUITE_DORICOLIB_PATH}}"; DestDir: "{autoappdata}\Ouaricon\Microtonal Suite"; Flags: ignoreversion
Source: "{{MICROTONAL_SUITE_README_PATH}}"; DestDir: "{autoappdata}\Ouaricon\Microtonal Suite"; Flags: ignoreversion
```

If full split-account support is required (admin elevates an installer launched by a separate non-admin), use `[Code]` to query `WTSQueryUserToken` for the active-console user and build the per-user roaming path explicitly. At minimum, document the elevating-account constraint in `inno-setup-creation.md` Section 3.4 so the README on Windows can warn users.

**Verification on a non-admin Windows account after fix:**
1. Log in as a non-admin user.
2. Launch the EXE; complete the UAC prompt with admin credentials.
3. After install, run `Get-Item "$env:APPDATA\Ouaricon\Microtonal Suite\Ouaricon-VST3-NoteExpression.doricolib"` from the **non-admin** session — file must exist at 6,431 B.
4. Open Dorico under the non-admin user → Library Manager → Import — must succeed without manual file-relocation.

---

## HIGH

### HI-01: `install-microtonal-suite.cmake.in` silently writes to filesystem root if `$ENV{HOME}`/`$ENV{APPDATA}` is empty

**File:** `modules/tuning/note-expression/install-microtonal-suite.cmake.in:27-34`
**Severity:** HIGH
**Issue:**
```cmake
if(APPLE)
    set(SHARED_DIR "$ENV{HOME}/Library/Application Support/Ouaricon/Microtonal Suite")
elseif(WIN32)
    set(SHARED_DIR "$ENV{APPDATA}/Ouaricon/Microtonal Suite")
```
If `HOME` is unset (CI, sudo without `-H`, daemon contexts) or `APPDATA` is unset (cross-compilation hosts, MSYS shells with mangled env), CMake silently substitutes the empty string. `SHARED_DIR` becomes `"/Library/Application Support/Ouaricon/Microtonal Suite"` on macOS or `"/Ouaricon/Microtonal Suite"` on Windows — both rooted at filesystem root.

Subsequent `file(MAKE_DIRECTORY "${SHARED_DIR}")` will then either:
1. Fail loudly with EACCES (best case),
2. Succeed under root and create a system-wide directory at `/Library/Application Support/Ouaricon/...` that the user cannot delete from a normal session (silent privilege-escalation footprint), OR
3. On Windows, attempt to create `\Ouaricon\Microtonal Suite` on the current drive root.

This is a developer-only path (the PKG postinstall on macOS bypasses CMake install and writes directly), but `cmake --install . --component ouaricon_note_expression_<TARGET>` is the documented developer flow per Plan 25-01 §`Canary Install Result` step 3. The script must fail loudly if the env var is unset rather than silently rewriting destination paths.

**Fix:**
```cmake
if(APPLE)
    if(NOT DEFINED ENV{HOME} OR "$ENV{HOME}" STREQUAL "")
        message(FATAL_ERROR "[Ouaricon] Microtonal Suite install: $HOME is unset; refusing to write to filesystem root")
    endif()
    set(SHARED_DIR "$ENV{HOME}/Library/Application Support/Ouaricon/Microtonal Suite")
elseif(WIN32)
    if(NOT DEFINED ENV{APPDATA} OR "$ENV{APPDATA}" STREQUAL "")
        message(FATAL_ERROR "[Ouaricon] Microtonal Suite install: %APPDATA% is unset; refusing to write to filesystem root")
    endif()
    set(SHARED_DIR "$ENV{APPDATA}/Ouaricon/Microtonal Suite")
else()
    message(STATUS "[Ouaricon] Microtonal Suite install: unsupported platform; skipping")
    return()
endif()
```

### HI-02: Postinstall heredoc relies on `sed` substitution of `PLUGIN_NAME_PLACEHOLDER` / `PRODUCT_NAME_PLACEHOLDER` with no validation; injection risk if plugin/product names contain `/` or `&`

**File:** `.claude/skills/plugin-packaging/references/pkg-creation.md:213-248`
**Severity:** HIGH
**Issue:**
The postinstall heredoc embeds `PLUGIN_NAME_PLACEHOLDER` and `PRODUCT_NAME_PLACEHOLDER` literals (lines 213, 221-222, 225-226, 233-234, 240, 242), then post-processes with `sed -i '' "s/PLUGIN_NAME_PLACEHOLDER/${PLUGIN_NAME}/g"` (line 247) and `s/PRODUCT_NAME_PLACEHOLDER/${PRODUCT_NAME}/g` (line 248). The sed `s` command treats the replacement string as having metacharacter semantics: any `/`, `&`, or `\` in `${PLUGIN_NAME}` or `${PRODUCT_NAME}` will either break the substitution or, worse, splice into the postinstall script as shell. While the current 8-cohort plugins all have safe names (alphanumeric + hyphen), the shared template in `pkg-creation.md` is the documented authoring pattern for **future** plugins; nothing in the doc constrains the names that may be plugged in.

Specifically, a plugin with `PRODUCT_NAME` containing `&` would be replaced with the **matched text** (sed's `&` substitution rule), expanding `cp -R "/tmp/PLUGIN_NAME_PLACEHOLDER/PRODUCT_NAME_PLACEHOLDER.vst3"` into something like `cp -R "/tmp/Foo/Foo&Bar.vst3"` — broken path. A name containing `/` would terminate the `s` command early and either error or splice. A name containing `$(...)` or backticks would be eligible for shell-expansion the next time the heredoc is re-emitted (the heredoc is single-quoted `'EOF'` so this specific case is safe today, but the pattern is fragile).

This is a templating convention and the postinstall runs as **root**, so the failure mode is privileged shell behavior driven by upstream string content.

**Fix:**
Two complementary mitigations:

1. Validate `PLUGIN_NAME` and `PRODUCT_NAME` against `^[A-Za-z0-9._-]+$` before emitting the heredoc; abort with an explicit error otherwise. Add to Section 1.2/1.3 of `pkg-creation.md` as a precondition.
2. Use a `sed` delimiter unlikely to appear in plugin names, AND escape replacement-side metacharacters:
```bash
# Validate first
[[ "$PLUGIN_NAME" =~ ^[A-Za-z0-9._-]+$ ]] || { echo "Invalid PLUGIN_NAME: $PLUGIN_NAME"; exit 1; }
[[ "$PRODUCT_NAME" =~ ^[A-Za-z0-9._\ -]+$ ]] || { echo "Invalid PRODUCT_NAME: $PRODUCT_NAME"; exit 1; }
# Then substitute with a delimiter that won't appear in names
sed -i '' "s|PLUGIN_NAME_PLACEHOLDER|${PLUGIN_NAME}|g" "$TEMP_DIR/scripts/postinstall"
sed -i '' "s|PRODUCT_NAME_PLACEHOLDER|${PRODUCT_NAME}|g" "$TEMP_DIR/scripts/postinstall"
```
Even better, use `envsubst` or generate the postinstall via a heredoc with `${VAR}` interpolation directly (drop the placeholder pattern entirely).

---

## MEDIUM

### ME-01: `install-microtonal-suite.cmake.in` lacks `cmake_minimum_required` — install-time CMake version drift could change `file(COPY)` behavior

**File:** `modules/tuning/note-expression/install-microtonal-suite.cmake.in`
**Severity:** MEDIUM
**Issue:**
The script is executed by `cmake --install` (or `installer -pkg`-driven cmake), which runs it as a fresh CMake script with whatever `cmake_minimum_required` the calling `module.cmake` happens to have set in the parent project. `file(COPY DESTINATION)` semantics differ across versions (e.g., 3.15 added `FOLLOW_SYMLINK_CHAIN`; 3.21 added `INPUT`; the underlying overwrite/permission default behavior has shifted). For a script that runs at install time on whatever machine has CMake installed (which may not match the build host), pinning the policy is defensive.

**Fix:**
Add at the top of the `.in` file:
```cmake
cmake_minimum_required(VERSION 3.22)
cmake_policy(VERSION 3.22)
```
This matches the version pinned in `OuariconModules.cmake:12` and protects against install-time CMake drift.

### ME-02: PKG postinstall blindly chowns entire `Ouaricon` parent dir, clobbering ownership of other Ouaricon assets installed previously

**File:** `.claude/skills/plugin-packaging/references/pkg-creation.md:235`
**Severity:** MEDIUM
**Issue:**
```bash
chown -R "$ACTUAL_USER:staff" "$USER_HOME/Library/Application Support/Ouaricon"
```
This recursively re-chowns **everything** under `~/Library/Application Support/Ouaricon/`, not just the Microtonal Suite subtree. If a future Ouaricon installer ships a different shared-path resource (e.g., a license cache, a sample bank, a credential file) under that parent and a user re-runs the v1.5 installer afterward, this line will chown that other content too — re-asserting `staff` group on whatever ownership/perms the other installer set. Today there's only one Ouaricon shared resource, so the chown is benign; once a second shared resource lands the line becomes a cross-resource interference vector.

**Fix:**
Scope the chown to the suite dir only:
```bash
chown -R "$ACTUAL_USER:staff" "$SUITE_DIR"
# Also chown the parent dir itself if newly created (mkdir -p creates it as root)
chown "$ACTUAL_USER:staff" "$USER_HOME/Library/Application Support/Ouaricon"
```
The parent-dir non-recursive chown handles the "we just created `~/Library/Application Support/Ouaricon` as root" case without recursing into siblings.

### ME-03: Inno Setup template-variable substitution does no path-quoting; absolute paths containing spaces break the `[Files]` Source: directive

**File:** `.claude/skills/plugin-packaging/references/inno-setup-creation.md:188-195, 199-204`
**Severity:** MEDIUM
**Issue:**
The PowerShell substitution snippets emit raw paths into the .iss file:
```powershell
$suiteDoricolib = "$repoRoot\modules\tuning\note-expression\resources\library\Ouaricon-VST3-NoteExpression.doricolib"
$issContent = $issContent -replace '\{\{MICROTONAL_SUITE_DORICOLIB_PATH\}\}', $suiteDoricolib
```
The .iss template wraps the variable in double quotes already (`Source: "{{MICROTONAL_SUITE_DORICOLIB_PATH}}"`), so paths containing spaces (`C:\Users\Some User\dev\repo\...`) survive. However, paths containing **double-quotes** (rare on Windows but legal in NTFS) or paths containing `\"` from PowerShell escape mishaps will break out of the quoting and corrupt the .iss compilation. There is no validation that `$repoRoot` resolves to a path safe for Inno Setup ingestion.

Additionally, `$issContent -replace` uses **regex** semantics — if the replacement string `$suiteDoricolib` contains `$` (rare on Windows, but `$` is legal in directory names), PowerShell's regex backreference syntax will eat it (`$&`, `$1`, etc.).

**Fix:**
1. Validate `$repoRoot` does not contain `"` or `$` characters before substitution; abort with explicit error otherwise.
2. Use literal-replace overload to bypass regex semantics:
```powershell
# Use [regex]::Escape on the search pattern (already escaped here)
# AND use [System.Text.RegularExpressions.Regex]::Replace's MatchEvaluator
# OR (cleanest) use the .Replace() string method, which is literal:
$issContent = $issContent.Replace('{{MICROTONAL_SUITE_DORICOLIB_PATH}}', $suiteDoricolib)
$issContent = $issContent.Replace('{{MICROTONAL_SUITE_README_PATH}}', $suiteReadme)
```
This is also more idiomatic and matches what Inno Setup template substitution should be — verbatim, not regex-mediated.

---

## LOW

### LO-01: `module.cmake` adds `install(SCRIPT)` unconditionally — even for plugin targets that exclude VST3, where Dorico-side asset is irrelevant

**File:** `modules/tuning/note-expression/module.cmake:57-63`
**Severity:** LOW
**Issue:**
The `configure_file` + `install(SCRIPT)` block at the bottom of `module.cmake` runs for **every** plugin that calls `ouaricon_add_module(<P> note-expression)`, regardless of whether the consuming plugin actually builds a VST3 target. Today all 8 cohort plugins do build VST3, but the module's own architecture (per the two-TU split documented in DOCS-01) explicitly supports AU-only / Standalone-only consumers where the `.doricolib` is dead weight (Dorico binds via VST3 only).

The asset is small (6,431 B), the install component is opt-in, and the install rule itself does no harm — but it pollutes the install components list and may lead a future operator to invoke `cmake --install --component ouaricon_note_expression_<P>` for a non-VST3 plugin and then wonder why Dorico still doesn't see the plugin. It also weakens the `.doricolib`-is-VST3-only invariant the rest of the codebase carefully maintains.

**Fix:**
```cmake
# Only attach the suite install rule when the consuming plugin builds a VST3 target.
if(TARGET "${TARGET_NAME}_VST3")
    configure_file(...)
    install(SCRIPT ...)
endif()
```

### LO-02: README and DOCS-02 cross-reference identical install-path strings; future path changes risk drift across files

**Files:**
- `modules/tuning/note-expression/README.md:179-180, 203-204`
- `modules/tuning/note-expression/resources/README-microtonal-suite.txt:24, 27`
- `research/microtonal-dorico-integration.md:265-266`
- `.claude/skills/plugin-packaging/references/pkg-creation.md:188, 231`
- `.claude/skills/plugin-packaging/assets/inno-template.iss:52-53, 81`
- `modules/tuning/note-expression/install-microtonal-suite.cmake.in:8-9, 28, 30`

**Severity:** LOW
**Issue:**
The strings `~/Library/Application Support/Ouaricon/Microtonal Suite/` (macOS) and `%APPDATA%\Ouaricon\Microtonal Suite\` (Windows) appear at least 7 distinct times across 6 files. There is no single source of truth — a v1.6 path change (e.g., to add a versioned subdirectory like `.../Microtonal Suite/v1/`) would require synchronized edits in all 6 files, with no automated check that they stay aligned. The `xmllint`-style schema sanity in DOCS-04 would not catch a path drift in DOCS-02.

**Fix:**
This is an organizational concern, not a correctness bug. Two options:
1. Document the path strings in a single canonical location (e.g., `25-CONTEXT.md` D-07) and have downstream references quote it as "(per D-07)" without repeating the literal path.
2. If literal paths must be repeated, add a CI grep-gate to assert all 6 files match a regex pattern, failing CI if drift is introduced.

For v1.5 ship, accept as-is and revisit in v1.6 alongside auto-discovery work.

---

## INFO

### IN-01: `module.cmake` line 18 hardcodes `/Users/taylorbrook/JUCE` as the macOS JUCE fallback path

**File:** `modules/tuning/note-expression/module.cmake:13-19`
**Severity:** INFO
**Issue:**
```cmake
if(DEFINED ENV{JUCE_DIR})
    set(_NE_JUCE_ROOT "$ENV{JUCE_DIR}")
elseif(WIN32)
    set(_NE_JUCE_ROOT "C:/JUCE")
else()
    set(_NE_JUCE_ROOT "/Users/taylorbrook/JUCE")
endif()
```
The macOS fallback is the project owner's personal home directory. This is pre-existing (not introduced in Phase 25) and per the phase scope I should not modify it, but I'm flagging it because the file is explicitly listed in `<phase_context>` files-to-review. For a multi-developer project this fallback fails on every other developer's machine; only the explicit `JUCE_DIR` env-var path actually works portably. The error message at line 35-37 does direct the user to `apply-juce-patches.sh`, but the underlying `_NE_JUCE_ROOT` is the wrong default.

**Fix (out-of-scope for Phase 25; v1.6+ candidate):**
Default to `${CMAKE_SOURCE_DIR}/../JUCE` (sibling-dir convention) or fail-fast with a clearer error than the marker-not-found case. This is also documented as a CLAUDE.md / MEMORY.md concern (the path appears in user memory). Recommend logging as a v1.6 cleanup task.

---

## What was Verified Clean (and how)

- **`.doricolib` XML:** `xmllint --noout` returns 0; `xmllint --xpath 'count(/kScoreLibrary/*)'` returns 48 (matches the load-bearing schema invariant from DOCS-03 quirk #4); file size is exactly 6,431 B; one occurrence each of `kVST3NoteExpression` and `xmap.ouaricon.vst3_note_expression`. Asset matches all string-pinning acceptance criteria from the plan.
- **Path A residue:** `grep -nE 'dorico_pt|tar cf|Default Library Additions|DefaultLibraryAdditions|ouaricon_extract_vst3_cids|DORICO_PT_STAGE|ouaricon_microtonal_suite_pt'` across all 12 in-scope files returns exactly one hit, in `research/microtonal-dorico-integration.md:481`, and that hit is intentional documentation that the helper is dead code (Path B narrative, not Path A residue).
- **CMake `configure_file` substitution surface:** Only `@CMAKE_CURRENT_LIST_DIR@` is substituted into the install script; no untrusted input crosses the build/install boundary. `${CMAKE_CURRENT_LIST_DIR}` resolves to `modules/tuning/note-expression/` at configure time (verified by tracing the `include()` from `OuariconModules.cmake:123`). The `@ONLY` mode prevents accidental `${VAR}`-style substitution.
- **PKG postinstall ACTUAL_USER discovery:** `stat -f '%Su' /dev/console` is the documented and correct macOS-postinstall pattern for resolving the GUI session user when the postinstall runs as root. The chown back to `$ACTUAL_USER:staff` correctly avoids leaving the suite asset root-owned.
- **README content / CLAUDE.md compliance:** The Phase 25 v3 docs preserve the "research goes in `research/`" convention, use `audience: internal-dev-only` front-matter (DOCS-05 boundary), and avoid auto-invocation of next-phase commands. No /clear or /implement misuse.
- **Plan tracking docs (`25-02-PREFLIGHT-AUDIT.md`, `25-02-VALIDATION-MATRIX.md`):** Are planning artifacts (markdown matrix tables); reviewed for accuracy of cross-references — they correctly cite the in-scope files and decisions.

---

_Reviewed: 2026-04-27T18:14:17Z_
_Reviewer: gsd-code-reviewer (standard depth)_
_Phase: 25-package-docs · v3 — Path B locked_
