# Headless Plugin UI Creation Workflow

**Purpose:** Complete protocol for detecting headless plugins and creating custom WebView UI via `/improve`.

**Context:** Plugins can skip custom UI during Stage 3 and ship as "headless" (DAW-provided controls only). Custom UI can be added later as a feature enhancement.

---

## Detection Logic

**When to check:** At the start of Phase 0.2 in plugin-improve workflow.

**Detection steps:**

1. **Read .continue-here.md:**
   ```bash
   cat plugins/[PluginName]/.continue-here.md
   ```

2. **Check for gui_type field:**
   - If `gui_type: headless` → Plugin is headless
   - If `gui_type: webview` → Plugin has custom UI (skip this workflow)
   - If field missing → Assume `webview` (backward compatibility)

3. **Fallback check (if field missing):**
   Check if WebView UI files exist:
   ```bash
   ls plugins/[PluginName]/Source/ui/public/index.html
   ```
   - If file doesn't exist → Plugin is headless
   - If file exists → Plugin has WebView UI

**Result:** Boolean - is plugin headless?

---

## Headless Detection Menu

**If headless detected, present:**

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Headless Plugin Detected
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[PluginName] is currently headless (no custom UI).

What would you like to do?

1. Create custom UI - Design WebView interface (becomes v1.1.0)
2. Keep headless and make other changes - Parameter tweaks, DSP fixes, etc.
3. Show me what headless means - Explain DAW controls
4. Other

Choose (1-4): _
```

**Handle responses:**
- **Option 1:** Proceed to "Create Custom UI Workflow" (below)
- **Option 2:** Skip to Phase 0.3 (normal /improve flow)
- **Option 3:** Display "Headless Explanation" (below), then re-present menu
- **Option 4:** Collect free-form text, reassess

---

## Headless Explanation (Option 3)

**Display this text, then wait for Enter:**

```
Headless Plugin Explanation
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

A "headless" plugin has no custom UI. Instead:

✓ DAW provides generic controls automatically:
  - Sliders for float parameters
  - Checkboxes for bool parameters
  - Dropdowns for choice parameters

✓ All parameters are exposed for automation

✓ Fully functional for mixing and production

✓ Smaller binary size (no WebView overhead)

Headless is ideal for:
- Utility plugins (gain, filters, limiters)
- Quick prototyping and testing
- Plugins where visual design isn't critical

You can add custom UI anytime via /improve [PluginName] → Create custom UI
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Press Enter to return to menu...
```

After user presses Enter, re-present "Headless Detection Menu" above.

---

## Create Custom UI Workflow (Option 1)

### Step 1: Confirmation Menu

**Present:**

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Creating Custom UI for [PluginName]
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

This will upgrade [PluginName] from headless to WebView UI.

Steps:
1. Design UI mockup (iterative, can refine)
2. Generate WebView integration code
3. Update version to v1.1.0 (minor version bump - new feature)
4. Build, test, and install

This will NOT break existing automation or presets.

Ready to proceed?

1. Yes, create UI - Start ui-mockup workflow
2. No, go back - Return to /improve menu
3. Learn more - What is WebView UI?
4. Other

Choose (1-4): _
```

**Handle responses:**
- **Option 1:** Proceed to Step 2 (Invoke ui-mockup)
- **Option 2:** Return to "Headless Detection Menu"
- **Option 3:** Display "WebView UI Explanation" (below), then re-present this menu
- **Option 4:** Collect free-form text, reassess

### Step 2: Invoke ui-mockup Skill

**Action:**

```
━━━ Invoking ui-mockup skill ━━━
```

Invoke ui-mockup skill using Skill tool:
```
Skill({ skill: "ui-mockup" })
```

**Wait for ui-mockup to complete.** User will iterate on mockup design until finalized.

**After mockup finalized:**

```
✓ UI mockup finalized

Now integrating WebView UI into [PluginName]...
```

### Step 3: Locate Finalized Mockup

**Find latest mockup:**
```bash
find plugins/[PluginName]/.ideas/mockups/ -name "*.html" -type f | sort -r | head -1
```

**If no mockup found:**
```
✗ Mockup finalization failed or was cancelled. Returning to menu.
```
Return to "Headless Detection Menu".

**If mockup found:** Store path as `MOCKUP_PATH` and proceed to Step 4.

### Step 4: Read Contracts

**Read design contracts:**

```bash
cat plugins/[PluginName]/.ideas/creative-brief.md
cat plugins/[PluginName]/.ideas/parameter-spec.md
cat plugins/[PluginName]/.ideas/architecture.md
```

Store contents for gui-agent invocation.

### Step 5: Invoke gui-agent Subagent

**Invoke gui-agent using Task tool:**

```
Task({
  subagent_type: "gui-agent",
  description: "Add WebView UI to [PluginName]",
  prompt: `
Integrate WebView UI for [PluginName] (upgrading from headless).

**Contracts:**
- creative-brief.md: [contents]
- parameter-spec.md: [contents]
- architecture.md: [contents]
- Mockup: [MOCKUP_PATH]

**Tasks:**
1. Read mockup and extract UI structure
2. Generate Source/ui/ directory with WebView resources
3. Update PluginEditor.h/cpp to use WebView (replace minimal editor)
4. Bind all parameters to UI controls
5. Update CMakeLists.txt (add JUCE_WEB_BROWSER=1, binary data)
6. Build and verify
7. Return JSON report with status

This is an upgrade from headless to WebView UI (v1.0.0 → v1.1.0).
  `
})
```

**Wait for gui-agent to complete.**

### Step 6: Parse gui-agent Result

**Expected result format:**
```json
{
  "status": "success" | "failure",
  "message": "...",
  "files_created": ["Source/ui/...", "..."],
  "parameters_bound": 5
}
```

**If status ≠ "success":**
```
✗ GUI integration failed. See error above.
```
Return to "Headless Detection Menu".

**If status = "success":** Proceed to Step 7.

### Step 7: Version Bump and State Updates

**1. Calculate new version:**
```bash
# Read current version from PLUGINS.md
CURRENT_VERSION=$(grep "^### $PLUGIN_NAME$" PLUGINS.md -A 2 | grep "Version" | awk '{print $4}')
# Bump minor version: 1.0.0 → 1.1.0
NEW_VERSION=$(echo $CURRENT_VERSION | awk -F. '{print $1"."($2+1)".0"}')
```

**2. Update .continue-here.md gui_type field:**
```bash
sed -i '' 's/^gui_type: headless/gui_type: webview/' plugins/[PluginName]/.continue-here.md
```

**3. Update PLUGINS.md table row:**
Update version and status using Edit tool:
- Find row: `### [PluginName]`
- Update Version: `[NEW_VERSION]`
- Update Status: `✅ Working` (if previously different)
- Update Last Updated: `[YYYY-MM-DD]`

**4. Update NOTES.md:**
If NOTES.md doesn't exist, create it. Then update:
- Status: `✅ Working`
- Version: `[NEW_VERSION]`
- Add timeline entry: "WebView UI added (v[NEW_VERSION]) - Custom interface with visual controls"

### Step 8: CHANGELOG Entry

**Get parameter count:**
```bash
PARAM_COUNT=$(grep "createParameterLayout" plugins/[PluginName]/Source/PluginProcessor.cpp -A 50 | grep "std::make_unique" | wc -l)
```

**Add CHANGELOG entry:**
```markdown
## [[NEW_VERSION]] - [YYYY-MM-DD]

### Added
- **WebView UI:** Custom interface with visual parameter controls
  - Replaces minimal editor (DAW controls still work)
  - Designed with ui-mockup skill
  - All [PARAM_COUNT] parameters bound to UI
  - VU meters and visual feedback added

### Changed
- **Binary size:** Increased due to WebView resources (expected)

### Notes
- Existing automation and presets are unaffected
- DAW generic UI still accessible as fallback
```

### Step 9: Git Commit

**Stage and commit changes:**
```bash
git add plugins/[PluginName]/Source/
git add plugins/[PluginName]/.continue-here.md
git add plugins/[PluginName]/CHANGELOG.md
git add PLUGINS.md

git commit -m "$(cat <<'EOF'
feat([PluginName]): add WebView UI (v[NEW_VERSION])

Upgraded from headless to custom WebView interface
All parameters bound to visual controls
Minor version bump (backward compatible)

🤖 Generated with Claude Code

Co-Authored-By: Claude <noreply@anthropic.com>
EOF
)"
```

### Step 10: Build and Install

**Build plugin:**
```bash
./scripts/build-and-install.sh [PluginName]
```

**Monitor build output.** If build fails, present error and return to menu.

### Step 11: Completion Menu

**Present:**

```
✓ [PluginName] v[NEW_VERSION] complete

UI upgrade successful:
- Headless (v[CURRENT_VERSION]) → WebView UI (v[NEW_VERSION])
- Custom interface integrated
- All parameters bound to visual controls
- Built and installed

What's next?

1. Test in DAW - Try out your new interface
2. Refine UI styling - Polish visual design
3. Make another improvement
4. Create new plugin
5. Other

Choose (1-5): _
```

**Handle responses:**
- **Option 1:** Display DAW testing instructions, wait
- **Option 2:** Provide UI refinement instructions
- **Option 3:** Re-run /improve (normal mode, not headless)
- **Option 4:** Suggest `/dream` or `/implement`
- **Option 5:** Ask what they'd like to do

**DAW testing instructions (Option 1):**
```
Instructions for DAW testing:
1. Load plugin in DAW
2. Test all UI controls
3. Verify parameter automation
4. Compare with DAW generic UI (for parity check)

Return here when ready.
```

**UI refinement instructions (Option 2):**
```
To refine UI:
1. Run /improve [PluginName]
2. Describe styling changes (colors, layout, fonts, etc.)
3. I'll update mockup and regenerate

Note: UI changes are PATCH version bumps (visual only).
```

---

## WebView UI Explanation (Option 3 in Confirmation Menu)

**Display this text, then re-present confirmation menu:**

```
WebView UI Explanation
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

WebView UI uses HTML/CSS/JavaScript to create custom plugin interfaces:

✓ Full visual control:
  - Custom layouts, colors, fonts
  - Animations and visual feedback
  - Branded, professional appearance

✓ Rapid iteration:
  - Design in browser first (ui-mockup skill)
  - Iterate on styling without recompiling
  - Modern web development tools

✓ All parameters bound to UI:
  - Knobs, sliders, toggles, dropdowns
  - Two-way sync (UI ↔ DSP)
  - Automation still works

Trade-offs:
- Larger binary size (+1-2MB due to WebView resources)
- Slightly higher CPU usage (minimal, unnoticeable)

Headless plugins are faster to build but less visually polished.
WebView plugins have custom branding and professional appearance.

Press Enter to return to menu...
```

---

## Integration Points

**Called from:** plugin-improve Phase 0.2

**Calls:**
- ui-mockup skill (Skill tool)
- gui-agent subagent (Task tool)
- build-automation skill (via scripts/build-and-install.sh)

**Updates:**
- .continue-here.md (gui_type field: headless → webview)
- PLUGINS.md (version, status)
- NOTES.md (version, status, timeline)
- CHANGELOG.md (version entry)
- Source files (gui-agent creates Source/ui/ directory, updates PluginEditor)

**Version impact:**
- Headless plugins are typically v1.0.0
- Adding WebView UI bumps to v1.1.0 (MINOR - new feature, backward compatible)
- UI refinements are PATCH bumps (v1.1.1, v1.1.2, etc.)

---

## Success Criteria

Custom UI creation is successful when:

- Mockup finalized via ui-mockup skill
- gui-agent completes without errors
- All parameters bound to UI controls
- Build succeeds
- .continue-here.md updated (gui_type: webview)
- PLUGINS.md updated (version, status)
- CHANGELOG.md updated (version entry with technical details)
- Git commit created with conventional format
- Plugin installed and testable in DAW
