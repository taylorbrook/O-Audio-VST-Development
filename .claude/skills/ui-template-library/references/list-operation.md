# Operation 3: List Aesthetics

Display all saved aesthetics in a formatted table with preview paths and decision menu.

**Input:** None (or optional filter by tag)
**Output:** Table of all aesthetics with prose summaries

---

## Step 1: Read Manifest

```bash
MANIFEST=".claude/aesthetics/manifest.json"
test -f "$MANIFEST" || { echo "No aesthetics saved"; exit 0; }
```

**Use Read tool to load manifest.json.**

---

## Step 2: Display Table

**Format as markdown table:**

```markdown
# Aesthetic Library

| ID | Name | Vibe | Source Plugin | Created | Used In |
|----|------|------|---------------|---------|---------|
| vintage-hardware-001 | Vintage Hardware | Warm retro analog | TapeAge | 2025-11-10 | TapeAge, CompCo |
| modern-minimal-002 | Modern Minimal | Clean flat blue/gray | EQPro | 2025-11-11 | EQPro |
```

**For each aesthetic:**
- ID
- Name
- Vibe (extract from aesthetic.md "Visual Identity" or metadata description)
- Source plugin
- Created date
- Used in plugins (comma-separated)

---

## Step 3: Show Preview Paths

```
Preview files:
- vintage-hardware-001: .claude/aesthetics/vintage-hardware-001/preview.html
- modern-minimal-002: .claude/aesthetics/modern-minimal-002/preview.html

Open in browser:
  open .claude/aesthetics/vintage-hardware-001/preview.html
```

---

## Step 4: Decision Menu

Present inline numbered menu (NOT AskUserQuestion tool):

```
What would you like to do?
1. View aesthetic details - Show full aesthetic.md prose
2. Apply aesthetic to plugin - Choose aesthetic and target
3. Delete aesthetic - Remove from library
4. Exit

Choose (1-4): _
```

**Wait for user response before proceeding.**
