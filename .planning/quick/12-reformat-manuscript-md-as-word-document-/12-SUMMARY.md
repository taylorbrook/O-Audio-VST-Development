---
phase: quick-12
plan: 01
status: completed
---

# Quick Task 12 Summary: Reformat manuscript as Word document

## What was done

Created a Python conversion script and generated two Word documents for Leonardo journal submission:

1. **Brook_ms.docx** — Full manuscript formatted per Leonardo guidelines
2. **Brook_cover-letter.docx** — Cover letter formatted consistently

### Manuscript (Brook_ms.docx)
- 105 paragraphs, all Normal style, Times New Roman 12pt
- 6 section headings with `<1>` bold prefix format
- Inline italics for all book/journal titles throughout body and references
- 45 numbered references with period format (Chicago 18th ed note style)
- 3 figure captions fully italicized
- Keywords section with bold label
- Abstract with inline italics
- Biographical Information placeholder
- Page: 8.5x11in, 1in margins

### Cover Letter (Brook_cover-letter.docx)
- Times New Roman 12pt, 1in margins
- *Leonardo* italicized throughout
- International date format (1 March 2026)
- Reviewer suggestions with en-dash bullets
- HTML author note stripped from output

## Files produced

| File | Location |
|------|----------|
| Conversion script | `/Users/taylorbrook/Documents/Articles/AI and the Culture Industry/submission/convert_manuscript.py` |
| Manuscript | `/Users/taylorbrook/Documents/Articles/AI and the Culture Industry/submission/Brook_ms.docx` |
| Cover letter | `/Users/taylorbrook/Documents/Articles/AI and the Culture Industry/submission/Brook_cover-letter.docx` |

## Verification

- Automated checks passed (all Normal style, zero font violations, correct page setup)
- User approved visual inspection in Word
