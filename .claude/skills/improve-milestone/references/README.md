# Reference Documentation

This directory contains reference materials for the improve-milestone skill.

## Contents

| File | Purpose |
|------|---------|
| `phase-agents.md` | Agent invocation specifications for each phase |
| `state-management.md` | Registry and STATUS.yaml update protocols |
| `version-integration.md` | Backup, CHANGELOG, and git tag procedures |
| `handoff-protocol.md` | Two-step format for phase transitions |

## Quick Reference

### Phase Cycle

```
discuss → research → plan → execute → verify
   ↓          ↓         ↓        ↓         ↓
CONTEXT.md RESEARCH.md PLAN.md SUMMARY.md VERIFICATION.md
```

### Agent Mapping

| Phase | Agent | Skippable |
|-------|-------|-----------|
| discuss | general-purpose | Yes |
| research | general-purpose | Yes |
| plan | general-purpose | No |
| execute | domain-specific | No |
| verify | general-purpose | Yes |

### State Files

- **Primary:** `plugins/[Name]/.planning/improvements/[slug]/STATUS.yaml`
- **Secondary:** `.planning/workflow/registry.json` (activeMilestone field)

## Related Assets

Check `../assets/` for phase output templates:
- `context-template.md`
- `research-template.md`
- `plan-template.md`
- `summary-template.md`
- `verification-template.md`
