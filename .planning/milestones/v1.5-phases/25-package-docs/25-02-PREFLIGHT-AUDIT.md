# Plan 25-02 Preflight: Per-Plugin Packaging Consumption Audit

Date: 2026-04-27
Auditor: GSD executor (Plan 25-02 Task 0)
Phase: 25-package-docs
Cohort: 8 plugins (O-Lyrica, O-Bells, O-IntonationPad, O-Prism, O-Wind, O-Reed, O-Bowed, O-Formant)

## Summary

| Plugin           | macOS Category | macOS Path | Windows Category | Windows Path |
| ---------------- | -------------- | ---------- | ---------------- | ------------ |
| O-Lyrica         | a              | none       | a                | none         |
| O-Bells          | a              | none       | a                | none         |
| O-IntonationPad  | a              | none       | a                | none         |
| O-Prism          | a              | none       | a                | none         |
| O-Wind           | a              | none       | a                | none         |
| O-Reed           | a              | none       | a                | none         |
| O-Bowed          | a              | none       | a                | none         |
| O-Formant        | a              | none       | a                | none         |

Total category-(c) BLOCKERS: 0
Total category-(b) orchestrators: 0
Total category-(a) shared-template consumers: 8 / 8 (both platforms)

## Audit Methodology

Two enumeration passes per platform, run from the project root:

**Per-plugin packaging-file enumeration:**
```bash
for p in O-Lyrica O-Bells O-IntonationPad O-Prism O-Wind O-Reed O-Bowed O-Formant; do
    find "plugins/$p" -maxdepth 4 -type f \( \
        -name "package*.sh" -o -name "package*.bash" -o -name "build-pkg*" -o \
        -name "postinstall" -o -name "Makefile" -o -name "package.json" -o \
        -name "*.iss" -o -name "build-installer*" -o -name "package*.ps1" \
    \) 2>/dev/null
    find "plugins/$p" -maxdepth 4 -type d \( \
        -name "dist" -o -name "packaging" -o -name "installer" -o -name "scripts" \
    \) 2>/dev/null
done
```

**Result:** zero matches across the cohort. No per-plugin `package*.sh`, no per-plugin `*.iss`, no per-plugin `postinstall`, no per-plugin `dist/`/`packaging/`/`installer/`/`scripts/` directory.

**Forked-postinstall-heredoc grep (category (c) detector):**
```bash
for p in O-Lyrica O-Bells O-IntonationPad O-Prism O-Wind O-Reed O-Bowed O-Formant; do
    grep -rln 'cat > .*postinstall\|cp -R "/tmp/.*\.vst3"\|chown -R "$ACTUAL_USER:staff"' "plugins/$p/" 2>/dev/null
done
```

**Result:** zero matches. No forked copies of the postinstall heredoc anywhere in the cohort.

**Total relevant-packaging-files count across cohort:**
```bash
find plugins/O-Lyrica plugins/O-Bells plugins/O-IntonationPad plugins/O-Prism \
     plugins/O-Wind plugins/O-Reed plugins/O-Bowed plugins/O-Formant \
     -maxdepth 4 -type f \( -name "*.iss" -o -name "package*.sh" -o -name "package*.ps1" \) 2>/dev/null | wc -l
```

**Result:** `0`. Drift detector for future audits — any non-zero count would surface a fork or per-plugin orchestrator and warrant re-running this preflight.

## macOS PKG details

Every cohort plugin is **category (a) — consumes shared reference directly**. The `/package <PluginName>` slash command (handler at `.claude/skills/plugin-packaging/SKILL.md`) reads `.claude/skills/plugin-packaging/references/pkg-creation.md` at packaging time and follows it as a procedural reference. There is no per-plugin packaging script anywhere in `plugins/<P>/`.

### O-Lyrica
- Category: a
- Path: none
- Excerpt: n/a

### O-Bells
- Category: a
- Path: none
- Excerpt: n/a

### O-IntonationPad
- Category: a
- Path: none
- Excerpt: n/a

### O-Prism
- Category: a
- Path: none
- Excerpt: n/a

### O-Wind
- Category: a
- Path: none
- Excerpt: n/a

### O-Reed
- Category: a
- Path: none
- Excerpt: n/a

### O-Bowed
- Category: a
- Path: none
- Excerpt: n/a

### O-Formant
- Category: a
- Path: none
- Excerpt: n/a

## Windows EXE details

Every cohort plugin is **category (a) — consumes shared reference directly**. The Windows EXE workflow (`.claude/skills/build-installer/SKILL.md`) substitutes `{{PLACEHOLDER}}` tokens in the shared template `.claude/skills/plugin-packaging/assets/inno-template.iss` at build time using PowerShell logic documented in `.claude/skills/plugin-packaging/references/inno-setup-creation.md`. No plugin has a hand-edited `.iss` in its source tree.

### O-Lyrica
- Category: a
- Path: none
- Excerpt: n/a

### O-Bells
- Category: a
- Path: none
- Excerpt: n/a

### O-IntonationPad
- Category: a
- Path: none
- Excerpt: n/a

### O-Prism
- Category: a
- Path: none
- Excerpt: n/a

### O-Wind
- Category: a
- Path: none
- Excerpt: n/a

### O-Reed
- Category: a
- Path: none
- Excerpt: n/a

### O-Bowed
- Category: a
- Path: none
- Excerpt: n/a

### O-Formant
- Category: a
- Path: none
- Excerpt: n/a

## Verdict

**GREEN — proceed to Tasks 1-3 unmodified.**

All 8 cohort plugins consume the shared installer templates directly via the `/package` (macOS PKG) and `build-installer` (Windows EXE) skill workflows. There are no forked postinstall heredocs, no hand-edited `.iss` files, and no per-plugin packaging orchestrators in the cohort source tree. Editing `.claude/skills/plugin-packaging/references/pkg-creation.md`, `.claude/skills/plugin-packaging/assets/inno-template.iss`, and `.claude/skills/plugin-packaging/references/inno-setup-creation.md` cascades to all 8 plugins on next installer rebuild.

**Cascade integrity confirmed.** Tasks 1-3 may proceed against the shared templates without scope expansion to per-plugin forks.

## Notes for Tasks 1-3

- The only `dist/` directory created at packaging time is per-`/package` invocation (skill produces `plugins/<P>/dist/` on the fly during a packaging run); none of these dirs exist in the working tree at audit time.
- The `${PROJECT_ROOT}` precondition added in Task 1 is consumed by the orchestrating shell context that drives `pkg-creation.md` Section 4a/4b. Today, the `/package` slash command runs from the repo root via Bash invocations, so `git rev-parse --show-toplevel` resolves correctly. Task 1 documents this as an explicit precondition.
- The `{{MICROTONAL_SUITE_DORICOLIB_PATH}}` and `{{MICROTONAL_SUITE_README_PATH}}` template variables added in Task 2 are resolved by the per-plugin Windows packaging PowerShell that already substitutes `{{VST3_SOURCE_PATH}}` and `{{OUTPUT_DIR}}`. Task 2 documents the same `Resolve-Path "${PSScriptRoot}\..\..\..\.."` pattern for the new variables.

---

*Phase: 25-package-docs*
*Plan: 25-02-installer-bundling-sweep Task 0*
*Verdict: GREEN*
*Audited: 2026-04-27*
