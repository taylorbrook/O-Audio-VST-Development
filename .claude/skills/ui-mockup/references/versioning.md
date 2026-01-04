# UI Mockup Versioning System

## Version Numbering

**Pattern:** v1, v2, v3, v4...

**Increments:**
- Start at v1 for first mockup
- Increment by 1 for each new design iteration
- No decimals (v1.1 is not allowed)

## File Naming Convention

**All 7 files share version prefix:**
- v[N]-ui.yaml
- v[N]-ui-test.html
- v[N]-ui.html (after finalization)
- v[N]-PluginEditor.h (after finalization)
- v[N]-PluginEditor.cpp (after finalization)
- v[N]-CMakeLists.txt (after finalization)
- v[N]-integration-checklist.md (after finalization)

**Example for v2:**
```
plugins/LushVerb/.ui/
├── v1-ui.yaml
├── v1-ui-test.html
├── v1-ui.html
├── v1-PluginEditor.h
├── v1-PluginEditor.cpp
├── v1-CMakeLists.txt
├── v1-integration-checklist.md
├── v2-ui.yaml         ← Current iteration
├── v2-ui-test.html    ← Current iteration
└── [v2 implementation files after finalization]
```

## Version Selection for Implementation

**Default behavior:** Use latest version number for Stage 3 integration.

**Explicit selection:** User can specify different version:
- "Use v2 design for implementation" → Integrate v2-ui.html
- "Finalize v1 instead" → Mark v1 as production, archive v2+

## Finalized vs Unfinalized Versions

**Finalized version:**
- Has all 7 files generated
- `mockup_finalized: true` in .continue-here.md
- `finalized_version: [N]` recorded
- Ready for Stage 3 integration

**Unfinalized version:**
- Has only 2 files (YAML + test HTML)
- User iteration in progress
- Not ready for Stage 3

## Version Cleanup

**When to clean up:**
- User finalizes v3, wants to remove abandoned v1 and v2
- User wants to start fresh from v1

**How to clean up:**
- Delete all v[N]-* files for unwanted versions
- Keep finalized version only
- Update .continue-here.md with new version tracking
