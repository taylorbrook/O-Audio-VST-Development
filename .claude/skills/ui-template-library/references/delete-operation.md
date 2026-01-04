# Operation 4: Delete Aesthetic

Remove aesthetic from library with confirmation and usage warning.

**Input:** Aesthetic ID
**Output:** Aesthetic removed from library

---

## Step 1: Confirm Deletion

**Show usage information:**
```
⚠️  Delete aesthetic "Vintage Hardware" (vintage-hardware-001)?

Currently used in 2 plugins:
- TapeAge
- CompCo

Deleting will NOT affect existing plugins (they have copies of the UI).
This only removes the template from the library.

Are you sure? (yes/no)
```

**Wait for explicit confirmation before proceeding.**

---

## Step 2: Remove Files

```bash
if confirmed; then
    rm -rf ".claude/aesthetics/$AESTHETIC_ID"
fi
```

**Removes**:
- aesthetic.md
- preview.html
- metadata.json
- Directory itself

---

## Step 3: Update Manifest

**Remove entry from manifest.json:**

1. Read manifest.json (Read tool)
2. Parse JSON and filter out deleted aesthetic from "aesthetics" array
3. Write updated manifest.json (Write tool)

**Bash implementation**:
```bash
# Read manifest.json
MANIFEST=".claude/aesthetics/manifest.json"
jq "del(.aesthetics[] | select(.id == \"$AESTHETIC_ID\"))" "$MANIFEST" > "$MANIFEST.tmp"
mv "$MANIFEST.tmp" "$MANIFEST"
```

---

## Step 4: Confirmation

```
✅ Aesthetic "Vintage Hardware" deleted from library.

Plugins using this aesthetic are unaffected (they have independent copies).

Remaining aesthetics: 1
```

**Present decision menu**:
```
What's next?
1. List remaining aesthetics
2. Save new aesthetic
3. Exit

Choose (1-3): _
```
