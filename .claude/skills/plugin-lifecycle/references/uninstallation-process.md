# Uninstallation Process

**Context:** This file is part of the plugin-lifecycle skill.
**Invoked by:** When user requests /uninstall-plugin [PluginName] (future command)
**Purpose:** Clean removal of plugin from system folders with cache clearing

---

## Step 1: Locate Plugin Files

Same as installation Step 2 (extract product name from CMakeLists.txt):

```bash
PRODUCT_NAME=$(grep 'PRODUCT_NAME' plugins/$PLUGIN_NAME/CMakeLists.txt | ...)

VST3_INSTALLED="$HOME/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3"
AU_INSTALLED="$HOME/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component"
```

Check existence:

```bash
VST3_EXISTS=false
AU_EXISTS=false

[ -d "$VST3_INSTALLED" ] && VST3_EXISTS=true
[ -d "$AU_INSTALLED" ] && AU_EXISTS=true
```

## Step 2: Confirm Removal

Ask user to confirm:

```
Remove [PluginName] from system folders?

This will delete:
- VST3: ~/Library/Audio/Plug-Ins/VST3/[ProductName].vst3 (2.4 MB)
- AU: ~/Library/Audio/Plug-Ins/Components/[ProductName].component (2.4 MB)

Source files in plugins/[PluginName]/ will NOT be deleted.
You can reinstall anytime with: /install-plugin [PluginName]

Proceed? (y/n): _
```

**If user confirms (y):** Proceed to Step 3
**If user cancels (n):** Exit uninstallation

## Step 3: Remove Files

Delete from system folders:

```bash
if [ "$VST3_EXISTS" = true ]; then
    rm -rf "$VST3_INSTALLED"
    echo "✓ VST3 removed"
fi

if [ "$AU_EXISTS" = true ]; then
    rm -rf "$AU_INSTALLED"
    echo "✓ AU removed"
fi
```

**Safety:**

- Only removes from system folders (`~/Library/Audio/Plug-Ins/`)
- Source code in `plugins/[PluginName]/` untouched
- Build artifacts in `build/` untouched

## Step 4: Clear Caches

Same as installation Step 6. See `references/cache-management.md`.

**Why cache clearing after uninstall:**

DAWs may still show removed plugin in list (from cache). Clearing forces rescan.

## Step 5: Update State Files

**Update PLUGINS.md table row:**
```bash
PLUGIN_NAME="[PluginName]"
DATE=$(date +%Y-%m-%d)

# Update status and last updated in table row
CURRENT_ROW=$(grep "^| ${PLUGIN_NAME} |" PLUGINS.md)
NEW_ROW=$(echo "$CURRENT_ROW" | awk -F'|' -v status=" ✅ Working " -v date=" ${DATE} " '{print $1 "|" $2 "|" status "|" $4 "|" date}')
sed -i '' "s/^| ${PLUGIN_NAME} | .*/$(echo "$NEW_ROW" | sed 's/[&/\]/\\&/g')/" PLUGINS.md
```

**Update plugins/[Name]/NOTES.md:**
```bash
NOTES_FILE="plugins/${PLUGIN_NAME}/NOTES.md"

if [ -f "$NOTES_FILE" ]; then
  # Update status
  sed -i '' "s/^- \*\*Current Status:\*\* .*$/- **Current Status:** ✅ Working/" "$NOTES_FILE"

  # Add timeline entry
  sed -i '' "/^## Lifecycle Timeline$/a\\
- **${DATE}:** Uninstalled from system folders
" "$NOTES_FILE"

  # Remove or comment out installation locations
  sed -i '' '/^\*\*Installation Locations:\*\*/,/^$/d' "$NOTES_FILE"
fi
```

**Verification (BLOCKING):**
```bash
# Verify PLUGINS.md table shows ✅ Working
TABLE=$(grep "^| ${PLUGIN_NAME} |" PLUGINS.md | awk -F'|' '{print $3}' | xargs)
echo "Table status: $TABLE"  # Should show: ✅ Working

# Check verification
if [ "$TABLE" != "✅ Working" ]; then
  echo "❌ PLUGINS.md update failed - status is '$TABLE' instead of '✅ Working'"
  echo "State corruption detected. Manual fix required."
  exit 1
fi

# Verify NOTES.md shows ✅ Working
if [ -f "plugins/${PLUGIN_NAME}/NOTES.md" ]; then
  NOTES_STATUS=$(grep "^\*\*Current Status:\*\*" "plugins/${PLUGIN_NAME}/NOTES.md" | sed 's/.*Current Status:\*\* //')
  echo "NOTES status: $NOTES_STATUS"  # Should show: ✅ Working

  if [ "$NOTES_STATUS" != "✅ Working" ]; then
    echo "❌ NOTES.md update failed - status is '$NOTES_STATUS' instead of '✅ Working'"
    echo "State corruption detected. Manual fix required."
    exit 1
  fi
fi

echo "✓ State verification passed"
```

## Step 6: Confirmation

```
✓ [PluginName] uninstalled

Removed from system folders:
- VST3: ~/Library/Audio/Plug-Ins/VST3/[ProductName].vst3 (deleted)
- AU: ~/Library/Audio/Plug-Ins/Components/[ProductName].component (deleted)

Cache status: Cleared (Ableton + Logic)

Source code remains: plugins/[PluginName]/

To reinstall: /install-plugin [PluginName]
```

---

**Return to:** Main plugin-lifecycle orchestration in `SKILL.md`
