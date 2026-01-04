# Git Operations for Plugin Planning

Standard git commit patterns for plugin-planning stages.

## Stage 0 Commit Pattern

**When to commit:** After architecture.md created, state updated, before presenting decision menu

**Files to stage:**
```bash
git add plugins/${PLUGIN_NAME}/.ideas/architecture.md \
        plugins/${PLUGIN_NAME}/.continue-here.md \
        PLUGINS.md
```

**Commit message format:**
```bash
git commit -m "$(cat <<'EOF'
feat: [PluginName] Stage 0 - research complete

🤖 Generated with Claude Code

Co-Authored-By: Claude <noreply@anthropic.com>
EOF
)"
```

**Variables to replace:**
- `[PluginName]` → Actual plugin name

## Stage 1 Commit Pattern

**When to commit:** After plan.md created, state updated, before presenting decision menu

**Files to stage:**
```bash
git add plugins/${PLUGIN_NAME}/.ideas/plan.md \
        plugins/${PLUGIN_NAME}/.continue-here.md \
        PLUGINS.md
```

**Commit message format:**
```bash
git commit -m "$(cat <<'EOF'
feat: [PluginName] Stage 1 - planning complete

Complexity: [X.X]
Strategy: [Single-pass | Phased implementation]

🤖 Generated with Claude Code

Co-Authored-By: Claude <noreply@anthropic.com>
EOF
)"
```

**Variables to replace:**
- `[PluginName]` → Actual plugin name
- `[X.X]` → Calculated complexity score
- `[Single-pass | Phased implementation]` → Determined strategy

## Commit Verification

After committing, verify:
```bash
git log -1 --oneline  # Shows last commit
git status            # Should show "nothing to commit, working tree clean"
```

## Rollback (if commit fails)

If commit fails due to pre-commit hooks or other issues:
```bash
git reset HEAD~1      # Undo last commit, keep changes staged
git status            # Review what's staged
# Fix issues, then retry commit
```
