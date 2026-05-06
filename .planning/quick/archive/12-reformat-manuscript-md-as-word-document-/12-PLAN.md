---
phase: quick-12
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - /Users/taylorbrook/Documents/Articles/AI and the Culture Industry/submission/convert_manuscript.py
  - /Users/taylorbrook/Documents/Articles/AI and the Culture Industry/submission/Brook_ms.docx
autonomous: false
requirements: [QUICK-12]

must_haves:
  truths:
    - "Brook_ms.docx exists and opens in Word"
    - "All manuscript content is present in correct order"
    - "Formatting matches Leonardo template conventions"
    - "Inline italics preserved for book/journal titles"
    - "References numbered with proper italic formatting"
  artifacts:
    - path: "/Users/taylorbrook/Documents/Articles/AI and the Culture Industry/submission/convert_manuscript.py"
      provides: "Markdown-to-docx conversion script"
    - path: "/Users/taylorbrook/Documents/Articles/AI and the Culture Industry/submission/Brook_ms.docx"
      provides: "Final formatted manuscript"
  key_links:
    - from: "convert_manuscript.py"
      to: "manuscript_template_0.docx"
      via: "python-docx Document() opens template for inherited styles/page setup"
    - from: "convert_manuscript.py"
      to: "manuscript.md"
      via: "reads and parses markdown content"
---

<objective>
Convert manuscript.md into a properly formatted Word document (Brook_ms.docx) following Leonardo journal submission requirements.

Purpose: Prepare the manuscript for submission to Leonardo journal, which requires specific .docx formatting (TNR 12pt, Normal style throughout, bold `<1>` section headings, italic labels, numbered references with proper typography).

Output: A Python conversion script and the final Brook_ms.docx file ready for submission.
</objective>

<execution_context>
@/Users/taylorbrook/.claude/get-shit-done/workflows/execute-plan.md
@/Users/taylorbrook/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@/Users/taylorbrook/Documents/Articles/AI and the Culture Industry/drafts/manuscript.md
@/Users/taylorbrook/Documents/Articles/AI and the Culture Industry/submission/manuscript_template_0.docx

<interfaces>
<!-- Template structure (all paragraphs use Normal style, TNR 12pt = 152400 EMU) -->
<!-- Page setup: 8.5x11in, 1in margins all around — inherited from template -->

Template paragraph order (after cover page which must be deleted):
- Paragraph [22]: "Article Type" (italic label)
- Paragraph [23]: Article type value (plain text, e.g. "Contemporary Scholarship")
- Paragraph [24]: "Title: Subtitle" (italic label)
- Paragraph [25]: Title text (plain)
- Paragraph [26]: "Author Names" (italic label)
- Paragraph [27]: Author names (plain)
- Paragraph [28]: "Author Name" info label (italic)
- Paragraph [29-30]: Author info entries (plain)
- Paragraph [32]: "Abstract" (bold)
- Paragraph [33]: Abstract text (plain)
- Paragraph [34]: "Keywords" (bold) + keyword text (plain) — same paragraph, two runs
- Paragraph [37+]: Section headings use "<1>Heading\n" (bold) followed by body text (plain) in SAME paragraph, or body in next paragraph
- Paragraph [53]: "References and Notes" (bold)
- Paragraph [54+]: Numbered references "1. ..." with book/journal titles in italic runs
- Paragraph [76]: "Biographical Information" (bold)
- Paragraph [77]: Instructions (italic)
- Paragraph [78]: Bio text (plain)

Key formatting rules from template inspection:
- ALL runs explicitly set font to Times New Roman, size 152400 EMU (12pt)
- Bold labels: "Abstract", "Keywords", "References and Notes", "Biographical Information"
- Italic labels: "Article Type", "Title: Subtitle", "Author Names", "Author Name (occupation)"
- Section headings: "<1>Heading Text\n" as bold run, body as plain run (same or next para)
- Figure captions: italic for "Alt Text:" label and artwork titles
- References: number + period + space, book/journal titles in italic runs
</interfaces>
</context>

<tasks>

<task type="auto">
  <name>Task 1: Create Python conversion script and generate Brook_ms.docx</name>
  <files>
    /Users/taylorbrook/Documents/Articles/AI and the Culture Industry/submission/convert_manuscript.py
    /Users/taylorbrook/Documents/Articles/AI and the Culture Industry/submission/Brook_ms.docx
  </files>
  <action>
Create a Python script using python-docx (already installed, v1.2.0) that:

1. Opens the template `manuscript_template_0.docx` to inherit page setup and styles
2. Clears ALL template paragraphs (delete all content, keep document structure/styles)
3. Builds the manuscript content in this exact order, all using Normal style with explicit TNR 12pt on every run:

**Article metadata section:**
- Italic run: "Article Type" (on its own line/paragraph)
- Plain paragraph: "General Article" (this is an update to a previous Leonardo article, fits General Article type)
- Blank paragraph
- Italic run: "Title" (on its own line)
- Plain paragraph: "Generative AI and the Culture Industry: Standardization, Authorship, and Resistance"
- Blank paragraph
- Italic run: "Author Names"
- Plain paragraph: "Taylor Brook"
- Blank paragraph
- Italic run: "Author info"
- Plain paragraph with italic formatting: "Taylor Brook (composer, researcher), INSERT AFFILIATION. Email: INSERT EMAIL. ORCID: INSERT ORCID."
- Blank paragraph

**Abstract section:**
- Paragraph with bold run "Abstract" only
- Paragraph with the abstract text from manuscript.md (the paragraph between "**ABSTRACT**" and the first "---"). The abstract must be rendered with inline italics for book titles wrapped in `*...*` in the markdown — parse these and create separate runs (italic for text between asterisks, plain for the rest).
- Blank paragraph
- Paragraph: bold run "Keywords:" + plain run " generative ai, standardization, culture industry, authorship, machine learning, adorno, surveillance capitalism"
- Blank paragraph

**Body sections:**
Parse each `## HEADING` from the manuscript. For each section:
- Create a paragraph with bold run: `<1>` + heading text (title case from the manuscript, but preserve original casing) + `\n` newline at end of bold run
- Then for each body paragraph under that heading, create a new paragraph with the text. CRITICAL: handle inline markdown italics `*text*` by splitting into runs — plain runs for non-italic text, italic runs for text between single asterisks. This is essential for book titles like *Dialectic of Enlightenment*, *Leonardo*, journal names, artwork titles, etc.
- For figure caption paragraphs (lines starting with `*Fig.`): create an italic paragraph. Within it, handle nested italic markers — the entire caption is italic per the manuscript's markdown (wrapped in `*...*`), but artwork titles within use double emphasis in the source. Render the full caption in italic. Artwork/book titles that appear within are already italic so no extra treatment needed.
- Preserve bracketed citations [1], [2], [1,2], etc. as-is in the text runs.
- Skip the `---` horizontal rule lines entirely.

**References section:**
- Paragraph: bold run "References and Notes"
- Blank paragraph
- For each reference line (lines starting with a number): create a paragraph. Parse each reference to apply italic formatting to book titles (text in `*...*` in the markdown) and journal/periodical names (also in `*...*`). The reference number format should be "N. " (number, period, space) followed by the reference content with appropriate italic runs for titles.
- Preserve URLs and DOIs as plain text.

**Biographical Information section:**
- Blank paragraph
- Paragraph: bold run "Biographical Information"
- Paragraph in italic: "Please add a 2-3 sentence biography."
- Plain paragraph: "TAYLOR BROOK is a composer and researcher. INSERT BIO TEXT."

**General formatting rules for ALL runs:**
- Every single run must explicitly set `run.font.name = 'Times New Roman'` and `run.font.size = Pt(12)`
- Use only the Normal style for all paragraphs
- No auto-numbering, no heading styles
- Single line spacing (do not set explicit line spacing — the template default is appropriate)

**Markdown italic parsing helper:**
Write a helper function `add_formatted_text(paragraph, text)` that:
- Uses regex to split text on `*...*` patterns (single asterisk italic markers)
- For each segment: if it was between `*...*`, create an italic run; otherwise create a plain run
- Each run gets explicit TNR 12pt
- Handle edge cases: asterisks at start/end of text, consecutive italic segments, escaped asterisks (unlikely in this manuscript)

**Reference parsing helper:**
Write `add_reference(paragraph, ref_text)` that:
- Strips the leading number and period (already handled by the caller)
- Calls `add_formatted_text` since book/journal titles are already marked with `*...*` in the manuscript markdown

4. Save to Brook_ms.docx
5. Run the script

Important: The `## References and Notes` heading in the manuscript uses a different format — references are lines starting with numbers (1, 2, 3...) without a period prefix in the markdown. The script should detect lines matching `^\d+\s+` as reference entries. Looking at the manuscript, references use the format "1 Taylor Brook, ..." (number, space, content) — prepend the period: output as "1. Taylor Brook, ..."
  </action>
  <verify>
    <automated>python3 -c "
from docx import Document
doc = Document('/Users/taylorbrook/Documents/Articles/AI and the Culture Industry/submission/Brook_ms.docx')
assert len(doc.paragraphs) > 50, f'Too few paragraphs: {len(doc.paragraphs)}'
styles = set(p.style.name for p in doc.paragraphs)
assert styles == {'Normal'}, f'Non-Normal styles found: {styles}'
from docx.shared import Pt
bad_runs = 0
for p in doc.paragraphs:
    for r in p.runs:
        if r.text.strip():
            if r.font.name != 'Times New Roman' or r.font.size != Pt(12):
                bad_runs += 1
assert bad_runs == 0, f'{bad_runs} runs missing TNR 12pt'
full_text = '\n'.join(p.text for p in doc.paragraphs)
assert 'Generative AI and the Culture Industry' in full_text
assert 'Abstract' in full_text
assert 'Keywords' in full_text
assert 'References and Notes' in full_text
assert 'Biographical Information' in full_text
assert '<1>' in full_text
s = doc.sections[0]
assert abs(s.page_width - 7772400) < 100, 'Wrong page width'
assert abs(s.top_margin - 914400) < 100, 'Wrong margins'
print('ALL CHECKS PASSED')
"</automated>
  </verify>
  <done>
    - Brook_ms.docx exists at the specified path
    - All paragraphs use Normal style
    - All runs are Times New Roman 12pt
    - Document contains: article type, title, author, abstract, keywords, all body sections with <1> headings, figure captions, references, biographical information
    - Inline italics correctly applied to book/journal titles throughout body and references
    - Page setup: 8.5x11in, 1in margins
    - Bracketed citations [1], [2] preserved
    - References numbered with periods (1. 2. 3.)
  </done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <name>Task 2: Verify Brook_ms.docx formatting in Word</name>
  <files>
    /Users/taylorbrook/Documents/Articles/AI and the Culture Industry/submission/Brook_ms.docx
  </files>
  <action>
    Open Brook_ms.docx in Microsoft Word and visually verify all Leonardo formatting requirements are met. Check document structure, font consistency, italic/bold formatting, section headings, references, and page setup.
  </action>
  <verify>User confirms document looks correct in Word</verify>
  <done>User approves the formatted manuscript or provides specific corrections</done>
</task>

</tasks>

<verification>
- Brook_ms.docx opens without errors in Word
- All content from manuscript.md is present
- Formatting matches Leonardo template conventions
- No Heading styles used — all Normal
- TNR 12pt throughout
</verification>

<success_criteria>
Brook_ms.docx is ready for Leonardo journal submission (pending user filling in affiliation/email/ORCID/bio placeholders). Document passes automated formatting checks and visual inspection confirms proper typography.
</success_criteria>

<output>
After completion, create `.planning/quick/12-reformat-manuscript-md-as-word-document-/12-SUMMARY.md`
</output>
