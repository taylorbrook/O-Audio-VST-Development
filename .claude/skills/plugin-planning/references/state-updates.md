# State Updates for Plugin Planning

Protocol for updating state files during plugin-planning stages.

## State Files

- `PLUGINS.md` - Global plugin registry
- `plugins/[Name]/.continue-here.md` - Plugin-specific continuation point

## Stage 0 State Updates

### Update .continue-here.md

Create or update handoff file:
```bash
cat > plugins/${PLUGIN_NAME}/.continue-here.md <<'EOF'
[Content from assets/continue-stage-0-template.md]
EOF
```

**Template variables to replace:**
- `${PLUGIN_NAME}` → Actual plugin name
- `${TIMESTAMP}` → Current timestamp (ISO 8601 format)

**Template location:** `assets/continue-stage-0-template.md`

### Update PLUGINS.md

**Action:** Update status to `🚧 Stage 0` and add timeline entry

**Using Edit tool:**
```
Find plugin section: "### [PluginName]"
Update Status line to: "Status: 🚧 Stage 0"
Append timeline entry: "- YYYY-MM-DD: Research complete (Stage 0)"
```

**Verification:**
```bash
grep -A 5 "^### ${PLUGIN_NAME}$" PLUGINS.md | grep "Stage 0"
# Should return: Status: 🚧 Stage 0
```

## Stage 1 State Updates

### Update .continue-here.md

Update handoff file:
```bash
cat > plugins/${PLUGIN_NAME}/.continue-here.md <<'EOF'
[Content from assets/continue-stage-1-template.md]
EOF
```

**Template variables to replace:**
- `${PLUGIN_NAME}` → Actual plugin name
- `${TIMESTAMP}` → Current timestamp
- `${COMPLEXITY_SCORE}` → Calculated complexity score (X.X format)
- `${PHASED_IMPLEMENTATION}` → "true" or "false"
- `${IMPLEMENTATION_STRATEGY}` → "Single-pass" or "Phased implementation"

**Template location:** `assets/continue-stage-1-template.md`

### Update PLUGINS.md

**Action:** Update status to `🚧 Stage 1` and add timeline entry

**Using Edit tool:**
```
Find plugin section: "### [PluginName]"
Update Status line to: "Status: 🚧 Stage 1"
Append timeline entry: "- YYYY-MM-DD: Planning complete (Stage 1) - Complexity: X.X"
```

**Verification:**
```bash
grep -A 5 "^### ${PLUGIN_NAME}$" PLUGINS.md | grep "Stage 1"
# Should return: Status: 🚧 Stage 1
```

## State Update Verification

After all updates:
```bash
# Verify .continue-here.md exists and is not empty
test -s "plugins/${PLUGIN_NAME}/.continue-here.md" || echo "ERROR: Handoff file missing or empty"

# Verify PLUGINS.md was updated
grep "Stage [01]" PLUGINS.md | grep "${PLUGIN_NAME}" || echo "ERROR: PLUGINS.md not updated"
```

## Rollback

If state updates fail:
```bash
# Restore from git (if previous version exists)
git checkout HEAD -- PLUGINS.md
git checkout HEAD -- "plugins/${PLUGIN_NAME}/.continue-here.md"
```
