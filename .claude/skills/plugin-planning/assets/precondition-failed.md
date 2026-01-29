# Precondition Failed

Plugin planning requires a creative brief to exist before proceeding.

**Missing:**
- `plugins/${PLUGIN_NAME}/.planning/BRIEF.md`

## What to do

1. **Create creative brief first:**
   - Use `/start` command to ideate plugin concept
   - Or use plugin-ideation skill to generate BRIEF.md

2. **If creative brief exists but not detected:**
   - Verify file path: `plugins/${PLUGIN_NAME}/.planning/BRIEF.md`
   - Check file permissions (must be readable)

3. **If resuming from checkpoint:**
   - Check .planning/STATUS.md for correct plugin name
