# PKG Creation Reference

Complete step-by-step implementation for creating branded PKG installers.

---

## Section 1: Prerequisites Verification

### 1.1 Check Plugin Status

```bash
# Verify plugin is installed
grep -A 5 "### $PLUGIN_NAME" PLUGINS.md | grep "Status:" | grep "📦 Installed"
```

If not found or status is different, abort with:
```
Error: Cannot package $PLUGIN_NAME - plugin not installed
Run: /install-plugin $PLUGIN_NAME
```

### 1.2 Extract PRODUCT_NAME

```bash
# Read PRODUCT_NAME from CMakeLists.txt
PRODUCT_NAME=$(grep 'PRODUCT_NAME' "plugins/$PLUGIN_NAME/CMakeLists.txt" | sed 's/.*PRODUCT_NAME "\(.*\)".*/\1/')
```

If empty, abort with:
```
Error: Could not extract PRODUCT_NAME from CMakeLists.txt
Check: plugins/$PLUGIN_NAME/CMakeLists.txt
```

### 1.3 Verify Binaries Exist

```bash
# Check VST3
if [ ! -d "$HOME/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3" ]; then
    echo "Error: VST3 not found at ~/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3"
    exit 1
fi

# Check AU
if [ ! -d "$HOME/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component" ]; then
    echo "Error: AU not found at ~/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component"
    exit 1
fi
```

---

## Section 2: Extract Plugin Metadata

### 2.1 Read PLUGINS.md Entry

Extract from plugin's section in PLUGINS.md:

```bash
# Extract version
VERSION=$(grep -A 2 "### $PLUGIN_NAME" PLUGINS.md | grep "Version:" | sed 's/.*Version:\*\* \(.*\)/\1/')

# Extract description (first line after "Description:")
DESCRIPTION=$(grep -A 1 "**Description:**" PLUGINS.md | tail -1)

# Count parameters
PARAM_COUNT=$(grep -A 100 "### $PLUGIN_NAME" PLUGINS.md | grep -E "^- [A-Z]+" | wc -l | xargs)
```

### 2.2 Read Parameter List

```bash
# Extract all parameters with details
PARAMS=$(grep -A 100 "### $PLUGIN_NAME" PLUGINS.md | sed -n '/\*\*Parameters/,/\*\*DSP:/p' | grep -E "^- ")
```

Store for use in ReadMe.txt template.

---

## Section 3: Create Branding Files

### 3.1 Setup Temp Directory

```bash
TEMP_DIR="/tmp/${PLUGIN_NAME}-installer"
rm -rf "$TEMP_DIR"
mkdir -p "$TEMP_DIR/resources"
mkdir -p "$TEMP_DIR/payload/${PLUGIN_NAME}"
mkdir -p "$TEMP_DIR/scripts"
```

### 3.2 Generate Welcome.txt

```bash
cat > "$TEMP_DIR/resources/Welcome.txt" << EOF
${PLUGIN_NAME} by TÂCHES

Welcome to the ${PLUGIN_NAME} installer.

This will install ${PLUGIN_NAME} v${VERSION}, ${DESCRIPTION}

What You're Installing:
• VST3 format (compatible with most DAWs)
• AU format (Logic Pro, GarageBand, and more)
• ${PARAM_COUNT} parameters for complete control

Click Continue to begin installation.
EOF
```

### 3.3 Generate ReadMe.txt

Use template from `assets/readme-template.txt`, populate with:
- Plugin name and version
- Full description
- Parameter list with ranges
- Installation locations
- Gatekeeper bypass instructions (copy from template)
- System requirements (macOS 14.6+)

Example structure:
```
${PLUGIN_NAME} by TÂCHES
Version ${VERSION}

ABOUT ${PLUGIN_NAME}

${DESCRIPTION}

PARAMETERS

${PARAMS}

INSTALLATION LOCATION

[...standard sections from template...]
```

### 3.4 Generate Conclusion.txt

```bash
cat > "$TEMP_DIR/resources/Conclusion.txt" << EOF
Installation Complete!

${PLUGIN_NAME} by TÂCHES has been successfully installed.

NEXT STEPS

1. Open your DAW (Logic Pro, Ableton, etc.)
2. Scan for new plugins if needed
3. Load ${PLUGIN_NAME} on any audio track
4. Remember to authorize in System Settings on first use

FIND ${PLUGIN_NAME} IN YOUR DAW

VST3: Look in your plugin browser under "TÂCHES" or "${PLUGIN_NAME}"
AU: Audio Units > Effect > TÂCHES > ${PLUGIN_NAME}

Thank you for using ${PLUGIN_NAME}!

— TÂCHES
EOF
```

---

## Section 4: Build Base Package

### 4a. Copy Binaries to Payload

```bash
cp -R "$HOME/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3" "$TEMP_DIR/payload/${PLUGIN_NAME}/"
cp -R "$HOME/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component" "$TEMP_DIR/payload/${PLUGIN_NAME}/"
```

### 4b. Create Postinstall Script

```bash
cat > "$TEMP_DIR/scripts/postinstall" << 'EOF'
#!/bin/bash
# Plugin Postinstall Script
# Copies plugins to user's Library folder

# Get the actual user (not root during installation)
ACTUAL_USER=$(stat -f '%Su' /dev/console)
USER_HOME=$(eval echo ~$ACTUAL_USER)

echo "Installing PLUGIN_NAME_PLACEHOLDER for user: $ACTUAL_USER"
echo "Home directory: $USER_HOME"

# Create plugin directories if they don't exist
mkdir -p "$USER_HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "$USER_HOME/Library/Audio/Plug-Ins/Components"

# Copy plugins
cp -R "/tmp/PLUGIN_NAME_PLACEHOLDER/PRODUCT_NAME_PLACEHOLDER.vst3" "$USER_HOME/Library/Audio/Plug-Ins/VST3/"
cp -R "/tmp/PLUGIN_NAME_PLACEHOLDER/PRODUCT_NAME_PLACEHOLDER.component" "$USER_HOME/Library/Audio/Plug-Ins/Components/"

# Set ownership
chown -R "$ACTUAL_USER:staff" "$USER_HOME/Library/Audio/Plug-Ins/VST3/PRODUCT_NAME_PLACEHOLDER.vst3"
chown -R "$ACTUAL_USER:staff" "$USER_HOME/Library/Audio/Plug-Ins/Components/PRODUCT_NAME_PLACEHOLDER.component"

# Clean up temp files
rm -rf "/tmp/PLUGIN_NAME_PLACEHOLDER"

echo "PLUGIN_NAME_PLACEHOLDER installation complete!"
exit 0
EOF

# Replace placeholders
sed -i '' "s/PLUGIN_NAME_PLACEHOLDER/${PLUGIN_NAME}/g" "$TEMP_DIR/scripts/postinstall"
sed -i '' "s/PRODUCT_NAME_PLACEHOLDER/${PRODUCT_NAME}/g" "$TEMP_DIR/scripts/postinstall"

chmod +x "$TEMP_DIR/scripts/postinstall"
```

### 4c. Run pkgbuild

```bash
cd "$TEMP_DIR"

pkgbuild \
    --root payload \
    --scripts scripts \
    --identifier "com.taches.$(echo $PLUGIN_NAME | tr '[:upper:]' '[:lower:]')" \
    --version "$VERSION" \
    --install-location /tmp \
    "${PLUGIN_NAME}-Installer.pkg"
```

Expected output:
```
pkgbuild: Inferring bundle components from contents of payload
pkgbuild: Adding component at [...]
pkgbuild: Adding top-level postinstall script
pkgbuild: Wrote package to [Name]-Installer.pkg
```

---

## Section 5: Build Branded Installer

### 5a. Create Distribution.xml

```bash
cat > "$TEMP_DIR/Distribution.xml" << EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>${PLUGIN_NAME} by TÂCHES</title>
    <organization>com.taches</organization>
    <domains enable_localSystem="true"/>
    <options customize="never" require-scripts="true" rootVolumeOnly="true" />

    <welcome file="Welcome.txt" mime-type="text/plain" />
    <readme file="ReadMe.txt" mime-type="text/plain" />
    <conclusion file="Conclusion.txt" mime-type="text/plain" />

    <pkg-ref id="com.taches.$(echo $PLUGIN_NAME | tr '[:upper:]' '[:lower:]')">
        <bundle-version/>
    </pkg-ref>

    <choices-outline>
        <line choice="default">
            <line choice="com.taches.$(echo $PLUGIN_NAME | tr '[:upper:]' '[:lower:]')"/>
        </line>
    </choices-outline>

    <choice id="default"/>
    <choice id="com.taches.$(echo $PLUGIN_NAME | tr '[:upper:]' '[:lower:]')" visible="false">
        <pkg-ref id="com.taches.$(echo $PLUGIN_NAME | tr '[:upper:]' '[:lower:]')"/>
    </choice>

    <pkg-ref id="com.taches.$(echo $PLUGIN_NAME | tr '[:upper:]' '[:lower:]')" version="${VERSION}" onConclusion="none">${PLUGIN_NAME}-Installer.pkg</pkg-ref>
</installer-gui-script>
EOF
```

### 5b. Run productbuild

```bash
cd "$TEMP_DIR"

productbuild \
    --distribution Distribution.xml \
    --resources resources \
    --package-path . \
    "${PLUGIN_NAME}-by-TACHES.pkg"
```

Expected output:
```
productbuild: Wrote product to [Name]-by-TACHES.pkg
```

---

## Section 6: Output Distribution Package

### 6a. Create Dist Directory

```bash
DIST_DIR="plugins/${PLUGIN_NAME}/dist"
mkdir -p "$DIST_DIR"
```

### 6b. Copy Installer

```bash
cp "$TEMP_DIR/${PLUGIN_NAME}-by-TACHES.pkg" "$DIST_DIR/"
```

### 6c. Generate install-readme.txt

```bash
cat > "$DIST_DIR/install-readme.txt" << EOF
${PLUGIN_NAME} v${VERSION} - Installation Instructions
===========================================

INSTALLATION:
1. Double-click "${PLUGIN_NAME}-by-TACHES.pkg"
2. Follow the installation prompts
3. The installer will copy ${PLUGIN_NAME} to your system folders:
   - VST3: ~/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3
   - AU: ~/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component

FIRST USE (IMPORTANT):
macOS will show a security warning because this plugin is not signed.

To bypass Gatekeeper:
1. Open your DAW (Logic Pro, Ableton, etc.)
2. When you first load ${PLUGIN_NAME}, macOS will block it
3. Go to System Settings > Privacy & Security
4. Scroll down and click "Open Anyway" next to ${PLUGIN_NAME} warning
5. Confirm you want to open it

You only need to do this once per plugin format (VST3 and AU).

PLUGIN INFO:
- Version: ${VERSION}
- Formats: VST3, AU
- Description: ${DESCRIPTION}

UNINSTALLATION:
To remove ${PLUGIN_NAME}:
1. Delete ~/Library/Audio/Plug-Ins/VST3/${PRODUCT_NAME}.vst3
2. Delete ~/Library/Audio/Plug-Ins/Components/${PRODUCT_NAME}.component
3. Restart your DAW

COMPATIBILITY:
- macOS 14.6 or later recommended
- Built for your Mac architecture

SUPPORT:
For questions or issues, contact TÂCHES.
EOF
```

### 6d. Get File Sizes

```bash
PKG_SIZE=$(du -h "$DIST_DIR/${PLUGIN_NAME}-by-TACHES.pkg" | cut -f1)
```

### 6e. Cleanup Temp Files

```bash
rm -rf "$TEMP_DIR"
```

### 6f. Display Success Summary

```
✓ ${PLUGIN_NAME} packaged successfully

Created: plugins/${PLUGIN_NAME}/dist/${PLUGIN_NAME}-by-TACHES.pkg (${PKG_SIZE})

Distribution package includes:
- ${PLUGIN_NAME}-by-TACHES.pkg (branded installer)
- install-readme.txt (installation guide)

To share with your friend:
1. Compress the dist/ folder to a ZIP
2. Send via email, Dropbox, or file sharing service
3. They double-click the PKG to install
```

---

## Testing Checklist

Before sending to friend, verify:

1. **PKG opens without errors** (double-click test)
2. **Installer shows branding** (Welcome screen has "by TÂCHES")
3. **ReadMe is readable** (no missing metadata)
4. **Installation completes** (no permission errors)
5. **Plugins appear in system folders** (check VST3 and AU)
6. **DAW can load plugin** (test in Logic or Ableton)

Test on a different user account if possible.

---

## Error Scenarios

### pkgbuild Fails

**Symptom:** "Error: Invalid payload structure"
**Cause:** Binaries not copied correctly
**Fix:** Verify VST3/AU exist in payload directory

### productbuild Fails

**Symptom:** "Error: Cannot read Distribution.xml"
**Cause:** XML syntax error or missing branding files
**Fix:** Validate XML, check resources/ directory has all .txt files

### Installer Doesn't Run

**Symptom:** "Cannot be opened because it is from an unidentified developer"
**Cause:** Gatekeeper blocking unsigned PKG
**Fix:** Right-click > Open (one-time bypass)

### Plugin Not Found After Install

**Symptom:** DAW doesn't show plugin after installation
**Cause:** Postinstall script failed or wrong user detected
**Fix:** Check Console.app for postinstall errors, verify manual installation

---

## Future Enhancements

- **DMG packaging:** Drag-and-drop style installer
- **ZIP packaging:** Simple archive, no installer
- **Code signing:** Integrate with Apple Developer account
- **Batch packaging:** Package multiple plugins at once
- **Custom icons:** Add branded icon to PKG file
