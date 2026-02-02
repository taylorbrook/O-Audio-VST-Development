# Plugin Workflow Handoff Protocol

## Purpose

Every workflow transition point MUST present a clean handoff that:
1. Shows what was completed
2. Presents the next command with full plugin name
3. Instructs user to `/clear` first
4. Lists alternative options

**Do NOT auto-proceed to the next workflow.** Present the handoff and STOP.

## Why Handoffs Matter

- Each workflow stage consumes significant context tokens
- Fresh context windows improve agent performance
- User controls pacing and can take breaks
- Clean separation prevents context pollution
- Commands with full plugin names are copy-paste ready

## Standard Format

**Two-step handoff is mandatory.** Users must clear context before starting the next workflow.

```
---

## ✓ [Stage/Phase] Complete

**[PluginName]** — [one-line description]

Files created/modified:
- `path/to/file1`
- `path/to/file2`

---

## ▶ Next Up

**[Next Stage]: [Name]** — [objective]

**Step 1:** `/clear` — fresh context window
**Step 2:** `/command [PluginName]`

---

**Also available:**

- `/alternative1 [PluginName]` → description
- `/alternative2 [PluginName]` → description
- Save for later (handoff file created)

---
```

## Plugin Workflow Handoff Points

**Every handoff is a two-step process: `/clear` first, then the next command.**

| After | Step 1 | Step 2 | Alternatives |
|-------|--------|--------|--------------|
| Ideation (BRIEF.md) | `/clear` | `/plan [Name]` | `/start [Name]` (mockup), save |
| Planning (ARCHITECTURE.md) | `/clear` | `/implement [Name]` | `/start [Name]` (mockup), save |
| Stage 1 (Foundation) | `/clear` | `/implement [Name]` | review, save |
| Stage 2 (DSP) | `/clear` | `/implement [Name]` | test, review, save |
| Stage 3 (GUI) | `/clear` | `/implement [Name]` | test, review, save |
| Stage 4 (Polish) | `/clear` | `/install-plugin [Name]` | test, package, save |

## Anti-Patterns

### DON'T: Auto-invoke next skill

```python
# BAD - consumes context, user loses control
Skill tool: plugin-planning
```

### DON'T: Present options without commands

```
# BAD - user has to figure out the command
What's next?
1. Plan the plugin
2. Create UI mockup
```

### DON'T: Omit /clear instruction

```
# BAD - context pollution
`/plan TapeAge`
```

### DON'T: Relegate /clear to a footnote

```
# BAD - /clear is easily missed
`/plan TapeAge`
<sub>`/clear` first → fresh context window</sub>
```

Use the explicit two-step format instead:
```
# GOOD - clear two-step sequence
**Step 1:** `/clear` — fresh context window
**Step 2:** `/plan TapeAge`
```

### DON'T: Use generic commands without plugin name

```
# BAD - user has to add plugin name
`/plan`
```

## Enforcement

All skills that complete a workflow stage MUST:

1. Check their `<handoff_required>` flag
2. Use the continuation format from this protocol
3. Include full plugin name in all commands
4. Present handoff and STOP (no skill invocation)

## Integration with STATUS.md

When presenting handoff, also update STATUS.md:

```markdown
## Resume Point

Handoff presented: [timestamp]
Next command: `/plan [PluginName]`
Context cleared: pending user action
```

This ensures `/continue` knows exactly where to resume.
