---
status: partial
phase: 25-package-docs
source: [25-VERIFICATION.md, 25-REVIEW.md (BL-01)]
started: 2026-04-27
updated: 2026-04-27
---

## Current Test

[awaiting human testing]

## Tests

### 1. Windows multi-account UAC elevation test (Inno Setup `{userappdata}` semantics)

expected: On a Windows machine with at least two user accounts (one standard user, one admin), install the EXE while signed in as the **standard user** and elevate UAC via the admin account's credentials. The bundled `.doricolib` + `README-microtonal-suite.txt` MUST land at the **standard user's** `%APPDATA%\Ouaricon\Microtonal Suite\`, NOT at the admin's. After install, launch Dorico 6 as the standard user and verify Library Manager Import succeeds (file is discoverable in the standard user's roaming profile). Quarter-sharp playback gate need not be re-tested.

result: [pending]

rationale: The D-08 canary ran on a single-account dev box where `{userappdata}` + `PrivilegesRequired=admin` + `PrivilegesRequiredOverridesAllowed` defaulting resolves to the launching admin's profile — which is also the dev's profile, so the test passed by coincidence. On a multi-account install, the elevating admin's `%APPDATA%` is wrong; the file becomes invisible to the user running Dorico. See `25-REVIEW.md` BL-01 for full analysis and the recommended fix (`DestDir: "{autoappdata}\\Ouaricon\\Microtonal Suite"` + `PrivilegesRequiredOverridesAllowed=dialog`).

fix_path: If FAIL, apply BL-01 patch to `.claude/skills/plugin-packaging/assets/inno-template.iss` (replace `{userappdata}` → `{autoappdata}`; add `PrivilegesRequiredOverridesAllowed=dialog`), rebuild + re-validate one Windows canary on a multi-account box. This becomes a Phase 25.1 gap-closure plan if the user wants to apply it before v1.5 ship.

## Summary

total: 1
passed: 0
issues: 0
pending: 1
skipped: 0
blocked: 0

## Gaps
