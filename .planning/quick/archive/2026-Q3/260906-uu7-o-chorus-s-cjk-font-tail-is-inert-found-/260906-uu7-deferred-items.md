# 260906-uu7 — deferred items

Recorded, not fixed. Each was discovered inside this task but sits outside the
charter the plan set ("Do not edit the three tail-less declarations"), so fixing
it here would have been unauthorized scope, not a Rule 1–3 auto-fix.

---

## D-uu7-1 — O-Chorus's three tail-less font stacks all reach the bare generic

**Where:** `plugins/O-Chorus/Source/ui/public/index.html` — `.preset-nav`,
`.preset-dropdown-item`, `.gear-btn` (pre-edit lines 130 / 233 / 404).

All three read `Garamond, 'Times New Roman', serif`. Garamond is not a macOS
face, and Times New Roman holds none of the glyphs these rules render, so the
bare `serif` generic **is** reached and hands each element a different fallback.
Measured by `CSS.getPlatformFontsForNode` on both the BEFORE and AFTER runs,
identically in en, fr and zh-Hans — this is not a language defect, it is the
same mechanism 260906-uu7 closed on the other six stacks, permanently:

| element | glyph | resolved face |
|---|---|---|
| `#preset-prev` | ◀ U+25C0 | Hiragino Mincho ProN / HiraMinProN-W3 (a Japanese serif) |
| `#preset-next` | ▶ U+25B6 | Lucida Grande / LucidaGrande |
| `.gear-btn` | ⚙ U+2699 | Menlo / Menlo-Regular (a monospace face) |

**It already has a visible consequence.** The two nav arrows are a matched pair
in the markup and are not one on screen, because they resolve through two
different faces:

```
#preset-prev  20.00 × 21.00 px  at y = 9.5
#preset-next  18.33 × 18.00 px  at y = 11.0
```

a 1.67 px width and 3.00 px height mismatch between ◀ and ▶.

**Why it was not fixed here:** the plan's Task 2 action says explicitly "Leave
the three tail-less declarations alone", and the v1.5.0 comment justifies each
by what it renders. Fixing it means naming a symbol-carrying face (or replacing
the glyphs with SVG), which is a visual-design decision, not a font-ordering
correction. Candidate for its own quick task.

**Where it is recorded in shipped source:** the rewritten `CJK TAIL — ORDER`
comment block in `index.html` carries this as *recorded-not-fixed*, and the
1.6.3 CHANGELOG entry carries it under `### Known / deferred` — so the next
reader of either file learns it without needing this file.

---

## D-uu7-2 — `.preset-dropdown-item`'s resolved face is UNMEASURED

The CDP probe reported `NO PLATFORM FONTS REPORTED` for it on **both** the
BEFORE and the AFTER run: the preset dropdown rendered no item under the probe's
gesture, so there was no node with a glyph run to interrogate.

This is an absence of evidence, not a clean result. Do not record
`.preset-dropdown-item` as clean anywhere. Closing it needs the probe extended
to open the dropdown (the way `tests/i18n-states.json` opens the settings
popover) — a probe change, which would have broken the before/after symmetry
this task's whole comparison rests on if made mid-task.

---

## D-uu7-3 — `measure-ui --report line-height-normal` cannot reach 0 on this page

Stands at **2** on both sides (`#preset-prev` / `#preset-next` at 14 px). It is
informational on a pinned page, consistent with the wave 4c finding. Treating it
as an acceptance criterion produces a chase with no end; the criterion used
throughout 260906-uu7 was `check-ui-labels` assertion 7 at 0 moved elements,
which is met. Not a defect — recorded so a later reader does not "fix" it.
