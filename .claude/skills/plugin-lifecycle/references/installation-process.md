# Installation Process

**Context:** This file is part of the plugin-lifecycle skill.
**Invoked by:** Main skill when user runs /install-plugin [PluginName]
**Purpose:** Complete installation process from build verification to system folder deployment

---

## Step 1: Build Verification

Check for build artifacts:

```bash
PLUGIN_NAME="[PluginName]"
PRODUCT_NAME="[ProductName]"

VST3_PATH="build/plugins/$PLUGIN_NAME/${PLUGIN_NAME}_artefacts/Release/VST3/${PRODUCT_NAME}.vst3"
AU_PATH="build/plugins/$PLUGIN_NAME/${PLUGIN_NAME}_artefacts/Release/AU/${PRODUCT_NAME}.component"

test -d "$VST3_PATH" && test -d "$AU_PATH"
```

**If binaries missing:** Offer to build Release mode now or abort.

## Step 2: Product Name Extraction

Extract PRODUCT_NAME from CMakeLists.txt (may differ from directory name):

```bash
PRODUCT_NAME=$(grep 'PRODUCT_NAME' plugins/$PLUGIN_NAME/CMakeLists.txt | sed 's/.*PRODUCT_NAME "\(.*\)".*/\1/')
```

**Why this matters:** Plugin directory may be `TapeAge` but product name is `Tape Age` (with space).

## Step 3: Old Version Removal

Check for existing installations:

```bash
VST3_INSTALLED="$HOME/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3"
AU_INSTALLED="$HOME/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component"
```

If old version found, remove before installing new version:

```bash
rm -rf "$VST3_INSTALLED" "$AU_INSTALLED"
```

**Why removal is important:** Prevents version conflicts and DAW cache issues.

## Step 4: Copy to System Folders

Install plugins to macOS system folders:

```bash
# VST3
cp -R "$VST3_PATH" ~/Library/Audio/Plug-Ins/VST3/

# AU
cp -R "$AU_PATH" ~/Library/Audio/Plug-Ins/Components/
```

**File structure after copy:**

```
~/Library/Audio/Plug-Ins/
├── VST3/
│   └── [Product Name].vst3/
│       └── Contents/MacOS/[Product Name]
└── Components/
    └── [Product Name].component/
        └── Contents/MacOS/[Product Name]
```

## Step 5: Permissions Verification

Set correct permissions (755):

```bash
chmod -R 755 "$HOME/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3"
chmod -R 755 "$HOME/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component"
```

**Why 755:** DAWs need read + execute permissions on plugin bundles.

Verify:

```bash
ls -ld "$HOME/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3"
# Expected: drwxr-xr-x
```

## Step 6: Cache Clearing

See `references/cache-management.md` for detailed DAW-specific cache clearing.

## Step 7: Verification

Check installed files:

```bash
ls -lh "$HOME/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3"
ls -lh "$HOME/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component"
```

**Verification checklist:**

- ✅ Files exist
- ✅ Permissions correct (drwxr-xr-x)
- ✅ Timestamps recent
- ✅ Sizes reasonable (> 100 KB typically)

**Installation Report:**

```
✓ [PluginName] installed successfully

Installed formats:
- VST3: ~/Library/Audio/Plug-Ins/VST3/[ProductName].vst3 (2.4 MB)
- AU: ~/Library/Audio/Plug-Ins/Components/[ProductName].component (2.4 MB)

Permissions: 755 (correct)
Timestamps: [Recent]

Cache status: Cleared (Ableton + Logic)

Next steps:
1. Open your DAW
2. Rescan plugins
3. Search for "[ProductName]"
4. Test audio processing and UI
```

## Step 8: Update State Files

**Update PLUGINS.md table row:**
```bash
PLUGIN_NAME="[PluginName]"
DATE=$(date +%Y-%m-%d)

# Update status and last updated in table row
CURRENT_ROW=$(grep "^| ${PLUGIN_NAME} |" PLUGINS.md)
NEW_ROW=$(echo "$CURRENT_ROW" | awk -F'|' -v status=" 📦 Installed " -v date=" ${DATE} " '{print $1 "|" $2 "|" status "|" $4 "|" date}')
sed -i '' "s/^| ${PLUGIN_NAME} | .*/$(echo "$NEW_ROW" | sed 's/[&/\]/\\&/g')/" PLUGINS.md
```

**Update plugins/[Name]/NOTES.md:**
```bash
NOTES_FILE="plugins/${PLUGIN_NAME}/NOTES.md"

# Create NOTES.md if missing
if [ ! -f "$NOTES_FILE" ]; then
  # Read version from PLUGINS.md
  VERSION=$(grep "^| ${PLUGIN_NAME} |" PLUGINS.md | awk -F'|' '{print $4}' | xargs)

  cat > "$NOTES_FILE" <<EOF
# ${PLUGIN_NAME} Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** ${VERSION}
- **Type:** Audio Plugin

## Lifecycle Timeline

- **${DATE}:** Installed to system folders (VST3 + AU)

## Known Issues

- None

## Additional Notes

**Installation Locations:**
- VST3: \`~/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3\`
- AU: \`~/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component\`

**Formats:** VST3, AU, Standalone
EOF
else
  # Update existing NOTES.md
  sed -i '' "s/^- \*\*Current Status:\*\* .*$/- **Current Status:** 📦 Installed/" "$NOTES_FILE"

  # Add timeline entry
  sed -i '' "/^## Lifecycle Timeline$/a\\
- **${DATE}:** Installed to system folders (VST3 + AU)
" "$NOTES_FILE"

  # Add or update installation details in Additional Notes
  if ! grep -q "Installation Locations:" "$NOTES_FILE"; then
    cat >> "$NOTES_FILE" <<EOF

**Installation Locations:**
- VST3: \`~/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3\`
- AU: \`~/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component\`
EOF
  fi
fi
```

**Verification (BLOCKING):**
```bash
# Verify PLUGINS.md table shows 📦 Installed
TABLE=$(grep "^| ${PLUGIN_NAME} |" PLUGINS.md | awk -F'|' '{print $3}' | xargs)
echo "Table status: $TABLE"  # Should show: 📦 Installed

# Check verification
if [ "$TABLE" != "📦 Installed" ]; then
  echo "❌ PLUGINS.md update failed - status is '$TABLE' instead of '📦 Installed'"
  echo "State corruption detected. Manual fix required."
  exit 1
fi

# Verify NOTES.md exists and shows 📦 Installed
NOTES_STATUS=$(grep "^\*\*Current Status:\*\*" "plugins/${PLUGIN_NAME}/NOTES.md" | sed 's/.*Current Status:\*\* //')
echo "NOTES status: $NOTES_STATUS"  # Should show: 📦 Installed

if [ "$NOTES_STATUS" != "📦 Installed" ]; then
  echo "❌ NOTES.md update failed - status is '$NOTES_STATUS' instead of '📦 Installed'"
  echo "State corruption detected. Manual fix required."
  exit 1
fi

echo "✓ State verification passed"
```

---

**Return to:** Main plugin-lifecycle orchestration in `SKILL.md`
