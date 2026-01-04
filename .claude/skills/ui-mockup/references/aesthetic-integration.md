# Aesthetic Integration

This document describes how ui-mockup skill integrates with ui-template-library skill for aesthetic management.

## Integration Points

### 1. Phase 0: Check for Aesthetic Library (Before Design Start)

Before starting design, check if aesthetics exist in the library.

**Check:**
```bash
if [ -f .claude/aesthetics/manifest.json ]; then
    AESTHETIC_COUNT=$(jq '.aesthetics | length' .claude/aesthetics/manifest.json)
    if [ $AESTHETIC_COUNT -gt 0 ]; then
        echo "Found $AESTHETIC_COUNT saved aesthetics"
    fi
fi
```

**If aesthetics exist, present decision menu:**
```
Found 3 saved aesthetics in library.

How would you like to start the UI design?
1. Start from aesthetic template - Apply saved visual system
2. Start from scratch - Create custom design
3. List all aesthetics - Browse library before deciding

Choose (1-3): _
```

**Option handling:**
- **Option 1**: Show aesthetic selector → Invoke ui-template-library "apply" operation
- **Option 2**: Skip to Phase 1.5 (free-form vision)
- **Option 3**: Invoke ui-template-library "list" operation → Show options again

### 2. Aesthetic Selection Menu

If user chose "Start from aesthetic template", display aesthetics:

```bash
# Read manifest
jq -r '.aesthetics[] | "\\(.id) | \\(.name) | \\(.vibe) | \\(.sourcePlugin)"' \
    .claude/aesthetics/manifest.json
```

**Display format:**
```
Available aesthetics:

1. Vintage Hardware (vintage-hardware-001)
   Vibe: Vintage analog
   Colors: Orange/cream/brown with paper texture
   Best for: Effects processors, time-based effects
   From: TapeAge mockup v2

2. Modern Minimal Blue (modern-minimal-002)
   Vibe: Clean, modern
   Colors: Blue/gray/white with subtle shadows
   Best for: Utility plugins, EQs, analyzers
   From: EQ4Band mockup v1

3. Aggressive Industrial (aggressive-industrial-003)
   Vibe: Dark, edgy
   Colors: Red/black/steel with metal texture
   Best for: Distortion, compression, aggressive effects
   From: HardClipper mockup v3

4. None (start from scratch)

Choose aesthetic: _
```

### 3. Apply Aesthetic Workflow

If user selects an aesthetic:

**Step 1: Load plugin context**
- Read `plugins/$PLUGIN_NAME/.ideas/creative-brief.md`
- Extract parameter count and types
- Identify prominent parameters (Mix, Output, etc.)

**Step 2: Invoke ui-template-library skill**
```
Invoke Skill tool with:
- skill: "ui-template-library"
- prompt: "Apply aesthetic [aesthetic-id] to [PluginName] with [N] parameters: [param list]"
```

**Step 3: Receive generated mockup**
- ui-template-library returns v1-ui-test.html with aesthetic applied
- Layout is appropriate for parameter count
- Visual styling matches aesthetic

**Step 4: Present for review**
```
✓ Mockup v1 generated with "Vintage Hardware" aesthetic

Files generated:
- plugins/ReverbPlugin/.ideas/mockups/v1-ui-test.html

Layout: 3x2 grid (6 knobs) + 2 vertical sliders (adaptive)
Colors: Orange/cream/brown (from aesthetic)
Styling: Tick-mark knobs, paper texture, generous spacing

What do you think?
1. This looks great - Finalize and create implementation files
2. Iterate on this design - Adjust colors/styling
3. Try a different aesthetic - Choose another template
4. Test in browser - Open v1-ui-test.html
5. Other

Choose (1-5): _
```

**Option handling:**
- **Option 1**: Proceed to Phase 5 (generate implementation scaffolding)
- **Option 2**: Allow manual edits → Increment version → Generate v2
- **Option 3**: Return to aesthetic selector
- **Option 4**: Open HTML in browser
- **Option 5**: Custom action

### 4. Phase 4.5: Design Decision Menu (After Manual Design)

**Original menu (for custom designs):**
```
✓ Mockup v[N] design created (2 files)

Files generated:
- v[N]-ui.yaml (design specification)
- v[N]-ui-test.html (browser-testable mockup)

What do you think?
1. Provide refinements (iterate on design)
2. Finalize and create implementation files
3. Test in browser
4. Other
```

**Enhanced menu (with aesthetic option):**
```
✓ Mockup v[N] design created (2 files)

Files generated:
- v[N]-ui.yaml (design specification)
- v[N]-ui-test.html (browser-testable mockup)

What do you think?
1. Provide refinements (iterate on design) ← Creates v[N+1]
2. Finalize and create implementation files (recommended if satisfied)
3. Save as aesthetic template (add to library for reuse)
4. Finalize AND save aesthetic (do both)
5. Test in browser (open v[N]-ui-test.html)
6. Validate WebView constraints (run checks)
7. Other

Choose (1-7): _
```

**New options:**
- **Option 3**: Invoke ui-template-library "save" operation
- **Option 4**: Save aesthetic first, then proceed to Phase 5

### 5. Save Aesthetic Workflow

If user chooses to save aesthetic:

**Step 1: Invoke ui-template-library skill**
```
Invoke Skill tool with:
- skill: "ui-template-library"
- prompt: "Save aesthetic from plugins/[PluginName]/.ideas/mockups/v[N]-ui.html"
```

**Step 2: ui-template-library prompts for details**
- Aesthetic name (suggested based on colors/vibe)
- Description (helps remember when to use it)
- Tags (for filtering)

**Step 3: Aesthetic saved**
```
✅ Aesthetic saved: "Vintage Hardware" (vintage-hardware-001)

Preview: .claude/aesthetics/vintage-hardware-001/preview.html

This aesthetic can now be applied to other plugins.
```

**Step 4: Return to decision menu**
```
What's next?
1. Continue to implementation (finalize mockup)
2. Start new plugin with this aesthetic
3. Exit
```

## Workflow Examples

### Example 1: First Plugin (Create and Save Aesthetic)

```
User: /ui-mockup TapeDelay
System: [No aesthetics exist yet]
System: [Phase 1-4: Create custom design]
System: [Phase 4.5 Decision Menu]
User: 3 (Save as aesthetic template)
System: [Invokes ui-template-library]
System: "What should we call this aesthetic?"
User: "Warm Vintage Tape"
System: ✅ Aesthetic saved
User: 1 (Continue to implementation)
System: [Phase 5-8: Generate implementation files]
```

### Example 2: Second Plugin (Apply Saved Aesthetic)

```
User: /ui-mockup ReverbPlugin
System: Found 1 saved aesthetic
System: "How would you like to start?"
User: 1 (Start from aesthetic template)
System: [Shows aesthetic: "Warm Vintage Tape"]
User: 1 (Apply this aesthetic)
System: [Invokes ui-template-library with 8 parameters]
System: ✅ Mockup generated (different layout, same visual style)
System: [Decision menu]
User: 1 (Finalize and create implementation files)
System: [Phase 5-8: Generate implementation files]
```

### Example 3: Explore Different Aesthetics

```
User: /ui-mockup Compressor
System: Found 3 saved aesthetics
User: 1 (Start from aesthetic template)
System: [Shows 3 aesthetics]
User: 1 (Warm Vintage Tape)
System: ✅ Mockup generated
User: 3 (Try different aesthetic)
System: [Shows 3 aesthetics again]
User: 3 (Aggressive Industrial)
System: ✅ Mockup generated (different vibe)
User: 1 (Finalize this one)
```

## Benefits of Integration

1. **Faster iteration** - Reuse visual systems across plugins
2. **Consistent brand** - Build visually unified plugin family
3. **Easier starting point** - Don't start from blank slate
4. **Discoverable** - Feature offered through decision menus
5. **Non-intrusive** - Option to skip and design from scratch
6. **Flexible** - Apply aesthetic, then customize further

## Technical Implementation

### File Dependencies

**ui-mockup reads:**
- `.claude/aesthetics/manifest.json` - To check if aesthetics exist
- `.claude/aesthetics/[id]/metadata.json` - To display aesthetic info

**ui-mockup writes:**
- Nothing (doesn't modify aesthetic library directly)

**ui-mockup invokes:**
- `ui-template-library` skill (via Skill tool) for:
  - List operations
  - Apply operations
  - Save operations

### State Management

**Phase 0 state:**
- `HAS_AESTHETICS` - Boolean, aesthetics exist in library
- `AESTHETIC_COUNT` - Number of aesthetics available
- `SELECTED_AESTHETIC_ID` - If user chose to apply aesthetic

**Phase tracking:**
- If `SELECTED_AESTHETIC_ID` is set:
  - Skip Phase 1.5 (free-form vision)
  - Skip Phase 2 (targeted questions)
  - Skip Phase 3 (generate YAML)
  - Start at Phase 4 (generate test HTML using aesthetic-generated HTML)

### Error Handling

**Aesthetic library missing:**
- Continue to Phase 1.5 (no option to use aesthetics)
- Inform user about aesthetic system after design creation

**Aesthetic application fails:**
- Fall back to Phase 1.5 (custom design)
- Preserve aesthetic ID for later retry

**Save aesthetic fails:**
- Don't block finalization
- User can save aesthetic later manually

## Future Enhancements

**Possible additions:**
- Auto-suggest aesthetic based on plugin type
- Preview aesthetic before applying (open preview.html)
- Tag-based filtering (show only "vintage" aesthetics)
- Aesthetic versioning (update aesthetic from improved mockup)
- Aesthetic inheritance (base new aesthetic on existing one)
