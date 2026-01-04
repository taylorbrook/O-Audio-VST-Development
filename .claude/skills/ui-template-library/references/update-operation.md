# Operation 5: Update Aesthetic

Refine aesthetic.md from improved mockup while preserving user edits.

**Input:**
- Aesthetic ID
- Path to improved mockup HTML

**Output:** Updated aesthetic.md with refined prose

---

## Overview

The update operation regenerates aesthetic descriptions from an improved mockup while attempting to preserve manual user edits to the prose.

**Use case**: User has improved the mockup after initial save and wants to update the aesthetic template without losing custom prose edits.

---

## Process

### Step 1: Load Existing Aesthetic

```bash
AESTHETIC_DIR=".claude/aesthetics/$AESTHETIC_ID"
test -d "$AESTHETIC_DIR" || { echo "Aesthetic not found: $AESTHETIC_ID"; exit 1; }
```

**Read**:
- Current aesthetic.md
- Current metadata.json
- Current preview.html (for comparison)

---

### Step 2: Analyze Improved Mockup

Run same analysis as save_aesthetic operation (see references/save-operation.md Steps 2-3):
1. Extract visual patterns from improved mockup
2. Generate new prose descriptions

---

### Step 3: Merge Prose (Preserve User Edits)

**Strategy**:
- Parse existing aesthetic.md section by section
- Regenerate each section from new mockup
- Compare old vs new prose
- Detect manual edits (non-template language, custom phrasing)

**When manual edits detected**:
```
Section "Knob Style" has manual edits. How to handle?
1. Keep existing (preserve manual edits)
2. Replace with regenerated (lose edits)
3. Show diff and let me merge

Choose (1-3): _
```

**Detection heuristics**:
- Custom vocabulary not in template
- Paragraph structure differs from template
- References to specific use cases
- Personal tone or emphasis

---

### Step 4: Update Metadata

```json
{
  "version": "1.1",  // Increment minor version
  "modified": "2025-11-12T15:30:00Z",
  "previousVersion": {
    "aesthetic": "aesthetic-v1.0.md",
    "preview": "preview-v1.0.html"
  }
}
```

**Version backups** (optional):
```bash
cp aesthetic.md aesthetic-v1.0.md
cp preview.html preview-v1.0.html
```

---

### Step 5: Replace Preview (Optional)

**Prompt**:
```
Replace preview.html with improved mockup?
1. Yes - Replace preview
2. No - Keep original preview
3. Keep both - Save as preview-v1.1.html

Choose (1-3): _
```

---

### Step 6: Commit Changes

```bash
git add .claude/aesthetics/$AESTHETIC_ID/
git commit -m "refactor(aesthetics): update $AESTHETIC_NAME to v1.1"
```

---

### Step 7: Confirmation

```
✅ Aesthetic "$AESTHETIC_NAME" updated to v1.1

Changes:
- aesthetic.md: 3 sections updated, 2 preserved (manual edits)
- preview.html: replaced
- metadata.json: version incremented

What's next?
1. View updated aesthetic.md
2. Apply updated aesthetic to plugin
3. List all aesthetics
4. Exit

Choose (1-4): _
```
