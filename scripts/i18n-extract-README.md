# `scripts/i18n-extract.js` — the in-scope-text inventory

Turns "localize this plugin" from transcription into review. Run it, read the
`UNSURE` and `DELETE` rows, paste the reviewed skeleton.

```bash
node scripts/i18n-extract.js --plugin O-Tapestop
node scripts/i18n-extract.js --all                       # size-ranked worklist
node scripts/i18n-extract.js --plugin O-Prism --out-dir /tmp/x
node scripts/i18n-extract.js --all --dry-run             # table only, no files
```

## Outputs

Written to `plugins/<Name>/.planning/` unless `--out-dir` says otherwise.

| File | What to do with it |
|---|---|
| `i18n-inventory.tsv` | One row per candidate. **Read every `UNSURE` and every `DELETE` row.** |
| `i18n-index-draft.html` | `index.html` with `data-i18n` added. `diff` it against the real file. |
| `i18n-labels-skeleton.js` | A `LABELS` block. Paste the reviewed entries into `js/i18n.js`, then delete. |

The draft is produced by inserting one attribute at the head of each start tag,
back-to-front so an earlier insertion cannot shift a later offset. Every
existing attribute keeps its order, its quoting and its line breaks — several
tags in this repo wrap across four lines, and rewriting them would bury a
one-attribute change in a whitespace diff.

The skeleton's French is the literal string `TODO`. `check-i18n` assertion 4
rejects a straight English passthrough and assertion 5 demands an explicit
`reviewed` flag, so an unfilled skeleton **cannot** pass.

## Three scanners, because the three sources have nothing in common

**1. HTML text nodes.** Parsed, not regexed. An attribute value containing `>`
and a `<` inside a comment both defeat a regex, and the parse also yields the
source offsets the draft needs. `<script>`, `<style>` and the document `<title>`
are excluded — the last is not visible in a WebView.

**2. Visible-text attributes** — `title`, `aria-label`, `placeholder`, `alt`.
Invisible to a text-node sweep and to a JS sweep alike. `aria-label` is the
accessible name; it is user-visible text by any definition that matters.

**3. JS-written strings.** The controller is `js/app.js` on most plugins and an
inline `<script type="module">` on O-Bitrot, so it is read from wherever it
lives — the same wrong-shaped assumption `check-i18n.js` already had to fix
once. All other `.js` under the UI root is scanned too, minus `js/juce/`,
`modules/`, the vendored `preset-manager.js` / `tuning-panel.js` /
`webview-drop-streaming.js`, and any `*.bundle.js`: editing a vendored copy here
would edit every plugin that embeds it.

**The whole right-hand side is read, not just a literal sitting against the
`=`.** That rule found 0 of O-Bitrot's 12 `textContent` sites, because the real
ones are `btn.textContent = on ? "On" : "Off"` and
`btn.textContent = btn.dataset.label || "Delete"`. Those conditionals are
precisely what contract §6 is about, so they are reported with a
`written from a CONDITIONAL expression` reason rather than dropped.

## Classification is a suggestion with a reason, never a silent drop

| Class | Meaning |
|---|---|
| `LABEL` | Localize. |
| `READOUT` | Numeric or unit-only — exempt under D-03, not touched. |
| `UNIT` | The whole string is a unit symbol. |
| `ENDONYM` | A language name; never translated. |
| `DELETE` | A native `title=` on an element that already has a `data-tip`. Contract §4: it renders a competing untranslated OS tooltip. |
| `UNSURE` | A human reads this row. |

Misfiling toward `LABEL` is cheap — an over-reported candidate costs one glance.
Misfiling toward `READOUT` ships English. So the `READOUT` rule is strict: a
string keeps `LABEL` unless, with its interpolations, numbers, punctuation and
known unit tokens removed, **no run of two letters survives**.

Two `UNSURE` reasons carry real design weight:

- **"the element holds element children as well as this text"** — keying the
  parent would make `applyLabel`'s `textContent` write delete those children.
  That is the systemic form of `pattern_js_state_updater_overwrites_html_labels`.
  Split the text into its own `<span>` first.
- **"this element DISPLAYS a preset name"** — exempt under D-02, because the
  name *is* the JSON filename. Confirm it is the name and not a caption beside
  it. The predicate is deliberately narrow: an earlier "sits near a preset
  control" version swept up O-Tapestop's `Save` / `Load` / `Delete` button
  captions, which are ordinary labels, and burying the one genuine row under
  three false ones is how a reviewer learns to skim `UNSURE`.

## Key naming

The element's own **id** wins. `document.querySelector` returns the first match
in document order, which is precisely how O-Octagon's `.vunit-group` tip nearly
landed on the wrong control in Stage C. Where an element has no id, the key is
scoped by its nearest id'd ancestor plus a slug of its text, so two identical
captions in different panels cannot collide. Uniqueness is checked across the
whole plugin.

**An existing tooltip key is REUSED where its English title already equals the
label text.** On most controls the tooltip title *is* the label, and two keys
holding the same string are two places for it to drift. Those rows are marked
`(reuses an existing entry)` and the skeleton emits a comment instead of a
duplicate entry.

## Measured totals, and where they disagree with the plan

`--all` over the tree as of Stage E:

| Source | Rows |
|---|---|
| HTML text nodes | 2,398 |
| Visible-text attributes | 296 |
| JS prose | 181 |
| JS composed (prose outside an interpolation) | 100 |
| **`LABEL` total** | **2,530** |
| `UNSURE` | 163 |
| `DELETE` | 4 |
| `READOUT` (exempt, D-03) | 234 |

Reconciliation against the numbers the plan carried:

- **Text nodes: 2,398 measured vs 2,438 planned.** The difference is almost
  exactly one per plugin, and it is the document `<title>`: 42 of the 43 pages
  carry one and O-Prism does not — which is why O-Prism's 247 matches the plan
  exactly while every other plugin comes in one lower. `<title>` is not visible
  in a WebView. **The extractor is right.**
- **`title=`: the plan said 196, the brief's correction said 120, this says
  108.** All three are correct about different things. A naive `grep title=`
  finds 196; **76** of those are `data-tip-title=`, leaving 120; of those 120,
  **108** are real HTML attributes on elements and **12** appear inside
  `<script>` blocks that build markup. Verified by counting each population
  separately.
- **`alt=`: the plan said 11, the brief said 28.** Again both: there are 28
  `alt` attributes and 11 of them carry prose. The other 17 are empty or a
  single glyph.
- **`aria-label=`: 172, exactly as planned.** No discrepancy.
- **`placeholder=`: 5, not 7.** Two of the seven live inside `<script>`.
- **JS strings: 281 rows vs the planned 53 + 27 = 80.** The extractor reports
  more, for three reasons, all deliberate: it scans every `.js` under the UI
  root rather than only the controller; it reads whole right-hand sides, so a
  ternary contributes both of its literals; and each literal is its own row.
  The planned figure counted *distinct prose strings*; this counts *sites*.
  Neither is wrong — but the site count is the one a localization pass has to
  work through.

  The receiver of a `textContent` write is **described, not matched**. An
  earlier version required it to be an identifier path and so missed
  `document.getElementById("x").textContent = "Free Run"` entirely — the
  parentheses are not in an identifier character class, and that is one of the
  two commonest shapes here. A `check-i18n` negative control caught it; the
  totals above are from after that fix, and are 37 rows higher than before it.
