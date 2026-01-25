# Release Notes Template

This template is used to generate GitHub Release descriptions.

## Variables

- `{{PLUGIN_NAME}}` - Plugin directory name (e.g., OuariconTremolo)
- `{{PRODUCT_NAME}}` - Human-readable name (e.g., Ouaricon Tremolo)
- `{{VERSION}}` - Semantic version (e.g., 1.4.0)
- `{{CHANGES}}` - Formatted changelog entry content
- `{{COMPANY_NAME}}` - From branding.json (e.g., Ouaricon Audio)
- `{{DATE}}` - Release date (YYYY-MM-DD)

---

## Template

```markdown
# {{PRODUCT_NAME}} v{{VERSION}}

{{CHANGES}}

---

## Downloads

| Platform | Format | Download |
|----------|--------|----------|
| macOS (Universal) | VST3 + AU | `{{PLUGIN_NAME}}-{{VERSION}}-macos.tar.gz` |
| Windows | VST3 | `{{PLUGIN_NAME}}-{{VERSION}}-windows.zip` |
| Linux | VST3 | `{{PLUGIN_NAME}}-{{VERSION}}-linux.tar.gz` |

## Installation

### macOS
```bash
# Extract and copy to plugin folders
tar -xzf {{PLUGIN_NAME}}-{{VERSION}}-macos.tar.gz
cp -R {{PRODUCT_NAME}}.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R {{PRODUCT_NAME}}.component ~/Library/Audio/Plug-Ins/Components/

# Clear AU cache
killall -9 AudioComponentRegistrar 2>/dev/null || true
```

**First launch:** Right-click → Open (Gatekeeper bypass for unsigned plugins)

### Windows
1. Extract ZIP file
2. Copy `.vst3` folder to `C:\Program Files\Common Files\VST3\`
3. Rescan plugins in your DAW

### Linux
```bash
tar -xzf {{PLUGIN_NAME}}-{{VERSION}}-linux.tar.gz
cp -R {{PRODUCT_NAME}}.vst3 ~/.vst3/
```

---

*Built with JUCE | {{COMPANY_NAME}} | {{DATE}}*
```

---

## Usage

The GitHub Actions workflow extracts the `## [VERSION]` section from CHANGELOG.md and uses it as the `{{CHANGES}}` content. The release notes are automatically populated when creating the GitHub Release.
