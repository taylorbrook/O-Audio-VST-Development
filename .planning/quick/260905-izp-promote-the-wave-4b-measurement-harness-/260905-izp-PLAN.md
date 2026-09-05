---
quick_id: 260905-izp
type: quick
description: "Promote the wave-4b measurement harness from scratch to scripts/ (D7)"
files_modified:
  - scripts/measure-ui.js
  - scripts/measure-ui-README.md
autonomous: true
estimate:
  tokens: 55000
  raw_tokens: 40000
  tasks: 3
  confidence: low
must_haves:
  truths:
    - "`node scripts/measure-ui.js --plugin O-Emulator` runs with no absolute path baked into the file and prints one JSON array of measured rows on stdout."
    - "Every DOM node measured gets its OWN row: O-AnalogEQ's 24 sibling SVG text nodes report as 24 rows, not 1."
    - "The four wave-4b defect screens run as `--report <screen>` over the same measurement, and each prints a count even when the count is 0."
    - "A screen reading 0 is distinguishable from a screen that did not run, because a positive control on the same data reports non-zero."
    - "The three measured method lessons (parsed shipping frame, cumulative state walk, OR-ed visibility) are stated in the file's own header in prose."
  artifacts:
    - scripts/measure-ui.js
    - scripts/measure-ui-README.md
  key_links:
    - "measure-ui.js -> scripts/serve-ui.js (buildRoot / serve / readEditorSize / resolvePlaywright / pluginRoot / REPO_ROOT)"
    - "measure-ui.js -> plugins/<Name>/Source/ui/public/js/i18n.js (LANGUAGES)"
    - "measure-ui.js -> plugins/<Name>/tests/i18n-states.json (cumulative walk; click | dblclick | eval)"
    - "measure-ui.js -> plugins/<Name>/Source/PluginEditor.cpp (setSize -> viewport)"
---

<objective>
Promote the wave-4b computed-style measurement harness out of a session
scratchpad into `scripts/measure-ui.js`, with the four defect screens it fed by
hand built in as `--report` filters, and a README in the style of
`check-ui-labels-README.md`.

Purpose: closes D7, the highest-value carry-forward from wave 4b. Four defect
classes in that wave were findable ONLY by computed style across every state at
the shipping frame. The harness that found them is uncommitted. Each wave
rebuilding it is waste; worse, each wave can rebuild it WRONG — wave 4b did,
twice (a non-cumulative state walk and a 1200px viewport, both of which hid real
defects present on the shipped page).

Output: `scripts/measure-ui.js` and `scripts/measure-ui-README.md`, committed
path-scoped, verified by running against two real plugins with positive controls
fired.

Not in scope: changing any plugin, editing any existing gate, promoting
`trace.js`, back-editing the wave-4b documents.
</objective>

<naming_decision>
**The script is `scripts/measure-ui.js`; the README is `scripts/measure-ui-README.md`.**

Chosen over `ui-computed-census.js` for two reasons. The `scripts/` family is
verb-first — `check-ui-labels.js`, `boot-all-uis.js`, `serve-ui.js`,
`check-i18n.js`, `gen-zh-trad-only.js` — and `measure-` sits in that family while
`ui-computed-census-` does not. And the verb carries the exit contract: `check-`
in this repo means a GATE (`check-ui-labels` exits `n>0` on `n` failures);
`measure-` does not. What is being promoted is a REPORT in exactly the sense
`boot-all-uis.js`'s header sets out, and the name should not invite a future
reader to wire it into CI as a gate. The `-README.md` suffix mirrors the two
existing script READMEs.
</naming_decision>

<context>
@.planning/quick/260905-acr-wave-4b/deferred-items.md
@scripts/serve-ui.js
@scripts/check-ui-labels-README.md

The scratch original is OUTSIDE the repo — read once, then supersede:
`/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/c6ebb30b-54c5-4328-8579-4e2c3f8c4cfd/scratchpad/measure.js`
</context>

<measured_at_planning_time>
Every number below was observed live during planning, not carried from a
document. Where a wave-4b document disagrees, THIS is the authority.

| Fact | Value | How observed |
|---|---|---|
| Playwright resolves | yes | `S.resolvePlaywright()` returned truthy |
| O-Emulator shipping frame | **620 x 430** | `readEditorSize('O-Emulator')`, from `plugins/O-Emulator/Source/PluginEditor.cpp:256` |
| O-IntonationPad frame / states | 800 x 500 / **19** | `readEditorSize`, `tests/i18n-states.json` |
| O-AnalogEQ frame / states | 920 x 220 / 2 | same |
| scratch harness, O-Emulator fonts | exit 0, 141 rows, 3 languages | ran it |
| distinct node ids per language, O-Emulator | **47** | 141 / 3 |
| `undeclared-font` on O-Emulator today | **0** | prototyped over the real rows |
| `line-height: normal` on O-Emulator visible Han LEAVES | **0** of 10 | prototyped over box-mode rows |
| `wrap-count` differing nodes on O-Emulator | **0** | prototyped over box-mode rows |
| O-AnalogEQ SVG text nodes in markup | **24** | `grep -o 'font-family="[^"]*"' plugins/O-AnalogEQ/Source/ui/public/index.html \| sort \| uniq -c` |
| O-AnalogEQ text rows the SCRATCH harness reports | **1** | ran the scratch harness with a `text` selector |
| CJK faces shipped repo-wide | `PingFang SC` 147, `Microsoft YaHei` 137, `Songti SC` 3 | grep census over `plugins/*/Source/ui/public` |
| state-file keys across all 43 files | `name` 229, `click` 169, `eval` 60, `dblclick` 1 | `cat plugins/*/tests/i18n-states.json \| grep -o '"[a-zA-Z]*":' \| sort \| uniq -c` |
| the one `dblclick` | O-Octagon | `grep -l '"dblclick"' plugins/*/tests/i18n-states.json` |
| `select` in any state file | **none** — a dead branch in the scratch file | same grep |
| git branch / staging at planning time | `main`, only `.gsd/dispatch-isolation-sentinel.json` modified | `git branch --show-current`, `git status --short` |

**A correction to the record, carried into the README rather than back-edited
into history:** wave 4b's SUMMARY and `deferred-items.md` both name O-Emulator's
shipping frame as "700 x 380". The frame parsed from that plugin's own
`PluginEditor.cpp` is **620 x 430**. The wrap-count LESSON stands unchanged — a
wide viewport hides the defect — but the number quoted alongside it does not.
Quote the parsed value in the README and note the discrepancy there. Leave the
wave-4b documents alone; they are a historical record.
</measured_at_planning_time>

<defects_to_fix_in_promotion>
Six, in descending order of cost. The first is not in the task brief and was
found by RUNNING the scratch file during planning.

1. **NODE IDENTITY COLLAPSES.** The scratch harness keys its output map on
   `lang + ' ' + id`, where `id` is `#id` or `tag.class.class`. Every node
   sharing a shape collapses into ONE row and the last visible one wins.
   **Measured: 24 sibling SVG text nodes on O-AnalogEQ report as 1 row.** On
   O-Emulator, 47 rows stand for a whole page — one row for every knob caption,
   one row for three language options. Every per-node count wave 4b produced was
   therefore a LOWER BOUND, and any screen run over such a key is structurally
   blind to siblings. Fix by keying on a DOM structural path, keeping the
   friendly string as a display field.
2. **The repo root is a hardcoded absolute path** to this machine's checkout.
   Take it from `serve-ui.js`, which already derives it from `__dirname`.
3. **`dblclick` states are silently skipped.** The scratch file handles
   `click` / `select` / `eval`; the shipped schema is `click` / `dblclick` /
   `eval`. A `dblclick` entry produces an empty step list and the state is a
   no-op with no message — O-Octagon's popover is measured as if it never
   opened. `select` is dead across all 43 files.
4. **Positional argv.** Every sibling takes `--plugin <Name>`.
5. **No usage line, no AGPL header, no design-notes header, no exit contract.**
6. **The viewport option is spelled RIGHT and must stay right.** The plural
   getter name is silently ignored as a launch option and leaves Chromium's
   1280x720 default. Say so in the header; that silent fallback is the mechanism
   behind lesson 1.
</defects_to_fix_in_promotion>

<tasks>

<task type="tracer">
  <name>Task 1: Promote the harness to scripts/measure-ui.js, working end to end</name>
  <files>scripts/measure-ui.js</files>
  <read_first>
    - the scratch original at the scratchpad path named in the context section above (6442 bytes)
    - `scripts/serve-ui.js` lines 1-95 (AGPL block + design header) and 543-590 (exports and CLI shape)
    - `scripts/check-ui-labels.js` lines 20-140 (design header, exit codes, `val()` argv helper) and 545-640 (how it reads and walks `tests/i18n-states.json`)
    - `scripts/boot-all-uis.js` lines 20-70 (the report-not-gate exit-code philosophy)
  </read_first>
  <action>
Create `scripts/measure-ui.js` from the scratch original, preserving its
measurement semantics exactly and repairing all six defects listed above.

HEADER — two blocks in the order every sibling uses. First the
AGPL-3.0-or-later block copied verbatim from `scripts/serve-ui.js` lines 1-19.
Then a design-notes block in the `/* ===== ... ===== */` frame, opening with a
one-line purpose. The notes MUST state in prose the three method lessons this
file exists to carry, because a reader who changes one of them must have been
told first:

  - THE SHIPPING FRAME IS PARSED, NEVER CHOSEN. The viewport comes from
    `readEditorSize()`, which reads `setSize(W,H)` out of that plugin's own
    `PluginEditor.cpp`. A wider frame measures a page nobody ships and the whole
    class of wrap-count defects disappears at it. Record the mechanism that
    makes this fragile: Playwright silently IGNORES the misspelled plural
    launch option and falls back to 1280x720, so one wrong identifier turns
    every measurement in this file into a measurement of a page that does not
    exist.
  - THE STATE FILE IS A CUMULATIVE WALK, NOT A SET OF ONE-SHOT CLICKS. The page
    loads ONCE per language and the states apply in file order without reset,
    because a state can require an earlier one — O-IntonationPad's rotation view
    is reachable only through the tuning tab. Resetting between states measures
    pages those states never produce.
  - THE VISIBILITY AND HAN FLAGS ARE OR-ED ACROSS STATES (W2). Keying a node on
    first sighting while walking cumulatively makes every node read as invisible
    for any panel the walk later navigates away from. On O-Prism that was 19
    visible-Han nodes found where 48 existed.

  Plus a fourth note, new to this promotion: NODE IDENTITY IS A DOM PATH, NOT A
  TAG-AND-CLASS STRING. Explain it with the measured number — a tag-and-class
  key reports O-AnalogEQ's 24 sibling SVG text nodes as one row, so every
  per-node count taken over such a key is a lower bound rather than a census.

REPO ROOT — `const REPO_ROOT = S.REPO_ROOT;`, taken from the module that already
derives it from `__dirname`. No path beginning with a user home directory
appears anywhere in the file — including in a comment. If the header credits the
scratch original, credit it as "wave 4b, quick task 260905-acr", never by
scratchpad path: a path that names one machine's home directory is exactly what
defect 2 is, and a header that carries it re-lands the defect as prose.

CLI — flags, using the same `val()` helper shape `check-ui-labels.js` uses:
`--plugin <Name>` (required), `--mode fonts|box` (default `fonts`),
`--select <css>` (restricts the sweep), `--root <DIR>` (repo-root override, as
`check-ui-labels.js` has), `--verbose` (diagnostics to stderr). With `--plugin`
absent, print the usage line and exit 2 — the shape both `check-ui-labels.js`
and `serve-ui.js` use.

EXIT CONTRACT — stated in the header as a table and honoured in code. This is a
REPORT, for the reason `boot-all-uis.js`'s header gives:
  `0`  the run completed — read stdout
  `1`  the harness itself could not run: no such plugin, no `setSize` in
       `PluginEditor.cpp`, no `js/i18n.js`, no `LANGUAGES` export
  `2`  usage error
  `77` Playwright unresolvable — NOTHING was measured, never a pass
A screen finding defects is NOT a non-zero exit. Say so in the header, because
the next reader's instinct will be to gate it.

OUTPUT — one JSON array on stdout and nothing else on stdout, so the file stays
composable with `jq` and node one-liners. Every human-readable line goes to
stderr: `--verbose` diagnostics, the identity disclosure, and the per-screen
counts Task 2 adds.

NODE IDENTITY — in the page-side `evaluate`, compute a stable structural key per
node by walking to `document.documentElement` and building an `nth-of-type`
chain. Emit it as `key`; keep the existing friendly string as `id` for reading;
key the accumulator on `lang + ' ' + key`. At end of run, print to stderr the
collapse that WOULD have happened — the count of distinct keys against the count
of distinct display ids, e.g. `identity: 141 nodes / 47 distinct display ids —
keyed by DOM path`. A reader comparing this run against a wave-4b count needs
that ratio to understand why the numbers moved.

STATE WALK — accept `click`, `dblclick` and `eval`: the three forms
`check-ui-labels.js` documents and the three the 43 shipped state files use. A
step whose selector does not resolve is a state this page does not have, so keep
the swallow — but under `--verbose` name the state and the selector on stderr,
because a silently skipped state is precisely how a `dblclick` entry passed for
a measured state in the scratch version.

MEASUREMENT FIELDS — keep every field the scratch version emits, in both modes.
Add exactly one: `ffAttr`, the node's own `font-family` presentation attribute
via `getAttribute`, or `null`. One field in the same evaluate pass is what makes
the SVG screen in Task 2 possible without a second sweep. Keep the Han regex
written as escaped code points so this file stays ASCII.

Do not change: the per-language goto + `__setLanguage` + settle sequence, the
timeouts, the carrier set (own text, `data-tip`, `data-tip-title`,
`aria-label`), the temp-dir copy of `i18n.js` for the ESM import, or the
teardown.
  </action>
  <verify>
    <automated>node scripts/measure-ui.js; test $? -eq 2 && node -e "const s=require('fs').readFileSync('scripts/measure-ui.js','utf8'); if(/\/Users\//.test(s)){console.error('FAIL: user-home path in file');process.exit(1)} if(!/AGPL-3\.0-or-later/.test(s)){console.error('FAIL: no AGPL header');process.exit(1)} if(!/viewport:\s*\{/.test(s)){console.error('FAIL: no viewport launch option');process.exit(1)} if(!/dblclick/.test(s)){console.error('FAIL: dblclick state form not handled');process.exit(1)} console.log('OK: usage exit 2, portable root, AGPL, viewport, dblclick')"</automated>
    <automated>node scripts/measure-ui.js --plugin O-AnalogEQ --mode fonts --select text > /tmp/izp-aeq.json && node -e "const r=require('/tmp/izp-aeq.json'); const en=r.filter(x=>x.lang==='en'); console.log('en text rows:',en.length); if(en.length!==24){console.error('FAIL: expected 24 sibling SVG text rows (scratch reported 1), got '+en.length);process.exit(1)} if(!en[0].ffAttr){console.error('FAIL: ffAttr not captured');process.exit(1)} console.log('OK: 24 rows, ffAttr present')"</automated>
    <automated>node scripts/measure-ui.js --plugin O-Emulator --mode box > /tmp/izp-emu-box.json && node -e "const r=require('/tmp/izp-emu-box.json'); const L=[...new Set(r.map(x=>x.lang))]; console.log('languages:',L.join(',')); if(L.length!==3){console.error('FAIL: expected 3 languages from the plugin i18n.js');process.exit(1)} const s=r.find(x=>x.vis&&x.kids===0); for(const f of ['key','id','lh','fsn','pt','pb','bt','bb','h']) if(s[f]===undefined){console.error('FAIL: box mode missing field '+f);process.exit(1)} console.log('OK: box mode carries every wrap-count field')"</automated>
  </verify>
  <done>`scripts/measure-ui.js` exists, carries the AGPL and design-notes headers with the four lessons in prose, contains no user-home path, exits 2 with a usage line when `--plugin` is absent, and emits per-node rows — 24 for O-AnalogEQ's SVG text siblings where the scratch version emitted 1 — in both modes across all three of the plugin's own languages.</done>
</task>

<task type="auto">
  <name>Task 2: Build the four wave-4b defect screens in as --report filters</name>
  <files>scripts/measure-ui.js</files>
  <action>
Add `--report <screen>` and `--from <file>` to `scripts/measure-ui.js`. This is
the half of D7's complaint that promoting the measurement alone does not fix:
wave 4b rebuilt the ANALYSIS by hand too, and the next wave will rebuild it
wrong the same way.

STRUCTURE — every screen is a PURE FUNCTION over the row array, taking
`(rows, opts)` and returning `{ name, count, findings, controlNote }`. Nothing
in a screen touches Playwright, the filesystem or the network. That is what
makes `--from <file>` work: `--report undeclared-font --from /tmp/rows.json`
runs the screen against a saved measurement with no browser at all, so a screen
can be re-run, diffed, and reviewed after the fact. When `--from` is given,
`--plugin` is not required.

Without `--report`, behaviour is unchanged: raw JSON on stdout. With `--report`,
the raw JSON still goes to stdout and the screen's findings go to STDERR as a
one-line count followed by the findings. **Print the count unconditionally,
including when it is 0** — a screen that prints nothing on a clean plugin is
indistinguishable from a screen that did not run. Accept `--report all` to run
every screen.

THE COUNT LINE HAS A FIXED FORMAT, because the verify blocks below match on it
and a paraphrase would make them lie:

    <screen-name>: <count> finding(s)

at the start of a line, one per screen, on stderr, before that screen's
findings. `svg-font-attr` prints its two counts on one such line:
`svg-font-attr: 24 attribute carrier(s), 0 finding(s)` — the carrier count
first, because that is the number that proves the screen ran.

MODE REQUIREMENTS. A screen declares the fields it needs and checks for them on
the first row. `box` mode is a superset of `fonts` mode, so the two font screens
run against either; the two geometry screens need `box`. A screen whose fields
are absent prints
`<screen-name>: SKIPPED — needs --mode box (field <name> not present)` and does
NOT print a `finding(s)` count, because a screen that cannot run must not be
mistaken for a screen that ran and found nothing. That distinction is the whole
point of printing counts at 0.

THE FOUR SCREENS.

`undeclared-font` — needs `ff`. Visible nodes carrying Han in any carrier
(own text, `data-tip`, `data-tip-title`, `aria-label`) whose computed
`font-family` names NO CJK face. **Take the face list from a named constant and
PRINT IT on every run**, so a reader can tell a clean result from a result
produced by an empty list. Seed the constant from the faces actually shipped in
this repo — `PingFang SC`, `Microsoft YaHei`, `Songti SC` — and allow
`--cjk-faces <comma,list>` to extend it. This is form 4 of the font-carrier
census (C2): `<button>`, `<select>` and `<input>` do not inherit `font-family`,
the UA stylesheet gives them one, and on this build that is Arial. A grep-based
census cannot see them at all, which is why this screen exists.

`line-height-normal` — needs `lh`, `kids`. Visible Han-carrying LEAF nodes (no
element children) whose computed `line-height` resolves to `normal`. Restrict to
leaves for the same reason `check-ui-labels` restricts its clip check to leaves:
a container's box does not report a text metric.

`wrap-count` — needs `h`, `pt`, `pb`, `bt`, `bb`, `lh`, `fsn`. Per node, lines are
`round((h - pt - pb - bt - bb) / lineHeightPx)`, where `lineHeightPx` is the
computed value, or `fsn * 1.2` when it resolves to `normal`. Report every node
whose line count differs between English and any non-English language, naming
the node, the language, and both counts. State the `1.2` fallback in the header
as an approximation of the UA's normal line box, and state that a node caught by
`line-height-normal` is one whose wrap count is estimated rather than measured
— the two screens are related, and the reader should be told which findings rest
on the estimate.

`svg-font-attr` — needs `ffAttr` and `ff`, so it runs in either mode, using the `ffAttr` field Task 1 adds. Report
in two parts, and print BOTH counts:
  (a) every node carrying a `font-family` presentation attribute — the raw
      count, which is the screen's own liveness signal;
  (b) the subset where the attribute DIVERGES from the computed stack.
Part (b) is the useful half and the reason this is a divergence report rather
than a presence report: a presentation attribute has the lowest specificity of
any font-family source, so a CSS rule overrides it silently. Measured during
planning on O-AnalogEQ: the 24 attributes read `Garamond, 'Times New Roman',
serif` while the computed stack reads the same list with the CJK tail inserted
before the generic. Someone reading only the markup concludes the tail is
missing; someone reading only the computed style never learns the markup
disagrees. Note in the header that those 24 carry no Han — they are numeric tick
labels — so a divergence here is a finding to READ, not a defect.

EXIT CODE — unchanged. A screen with findings still exits 0. `--report` with an
unknown screen name exits 2 with the list of valid names.
  </action>
  <verify>
    <automated>node scripts/measure-ui.js --report no-such-screen --from /tmp/izp-emu-box.json; test $? -eq 2 && echo "OK: unknown screen exits 2"</automated>
    <automated>node scripts/measure-ui.js --report all --from /tmp/izp-emu-box.json > /dev/null 2>/tmp/izp-emu-report.txt; echo "exit=$?"; cat /tmp/izp-emu-report.txt; grep -q 'undeclared-font' /tmp/izp-emu-report.txt && grep -q 'line-height-normal' /tmp/izp-emu-report.txt && grep -q 'wrap-count' /tmp/izp-emu-report.txt && grep -q 'svg-font-attr' /tmp/izp-emu-report.txt && echo "OK: all four screens printed a count from a saved file, no browser"</automated>
    <automated>node scripts/measure-ui.js --report svg-font-attr --from /tmp/izp-aeq.json 2>&1 >/dev/null | tee /tmp/izp-aeq-svg.txt; node -e "const t=require('fs').readFileSync('/tmp/izp-aeq-svg.txt','utf8'); if(!/\b24\b/.test(t)){console.error('FAIL: svg-font-attr did not report 24 attribute carriers — the screen is not running or identity collapsed again');process.exit(1)} console.log('OK: 24 attribute carriers reported — the screen is live')"</automated>
    <automated>node -e "const r=require('/tmp/izp-emu-box.json'); const f=r.filter(x=>x.vis&&x.han); console.log('independent scan: visible Han nodes on O-Emulator =',f.length); if(f.length===0){console.error('FAIL: nothing Han-bearing measured — a 0 from any screen would be vacuous');process.exit(1)} console.log('OK: the screens had non-empty input, so a 0 is a result')"</automated>
  </verify>
  <done>`--report {undeclared-font,line-height-normal,wrap-count,svg-font-attr,all}` and `--from <file>` work; each screen prints its count to stderr even at 0; `undeclared-font` prints the CJK face list it used; `svg-font-attr` reports 24 attribute carriers on O-AnalogEQ; raw JSON still lands on stdout; an unknown screen name exits 2.</done>
</task>

<task type="auto">
  <name>Task 3: Write measure-ui-README.md, fire the controls, commit path-scoped</name>
  <files>scripts/measure-ui-README.md, scripts/measure-ui.js</files>
  <read_first>
    - `scripts/check-ui-labels-README.md` in full — this is the documentation style to match
    - `.planning/quick/260905-acr-wave-4b/deferred-items.md` sections D7, C2, C3
  </read_first>
  <action>
Write `scripts/measure-ui-README.md` in the shape of
`check-ui-labels-README.md`: an opening line naming what it retires (here: D7),
the invocation block, an exit table, then the reasoning sections. Sections to
carry, each with its evidence:

  - **Exit table** — 0 / 1 / 2 / 77, with the sentence that a screen finding
    defects is a 0. State plainly that this is a report and not a gate, and why
    (`boot-all-uis.js`'s argument: a permanently-red gate destroys the habit of
    reading gates).
  - **The frame is parsed, never chosen.** Quote the value the tool parses:
    O-Emulator is **620 x 430**, read from its own `PluginEditor.cpp`. Record
    that wave 4b's SUMMARY and deferred-items name "700 x 380" for that plugin,
    that the parsed frame disagrees, and that the LESSON is unaffected — a wide
    viewport hides wrap-count defects either way. Also record the mechanism: the
    misspelled plural launch option is silently ignored and leaves 1280x720.
  - **The state walk is cumulative.** O-IntonationPad's rotation view needs the
    tuning tab first; a walk that resets between states measures a page those
    states never produce. Name the three step forms and note that `dblclick`
    appears exactly once across the 43 state files (O-Octagon) — which is
    precisely why the scratch version could omit it and look correct.
  - **Visibility and Han are OR-ed across states (W2).** With the O-Prism
    number: 19 found where 48 existed.
  - **Identity is a DOM path.** The measured evidence: the scratch key reported
    O-AnalogEQ's 24 sibling SVG text nodes as 1 row and O-Emulator's whole page
    as 47 rows. Say what that means for reading a wave-4b count: those counts
    were lower bounds.
  - **The four screens**, one subsection each, with what each caught in wave 4b:
    form 4 of the font-carrier census (13 nodes on O-IntonationPad, 1 on
    O-AnalogEQ — found only by computed style, invisible to any grep); the
    `line-height: normal` leaves; O-Emulator's engraved plate wrap count; and the
    SVG presentation-attribute divergence.
  - **Reading a 0.** The wave already repaired these plugins, so
    `undeclared-font`, `line-height-normal` and `wrap-count` all read 0 on
    O-Emulator today. Print the positive-control invocation that distinguishes
    that 0 from a dead screen, and its expected output.
  - **Known limitation, carried openly** — in the style of that section in
    `check-ui-labels-README.md`. At minimum: the `1.2` normal-line-box
    approximation in `wrap-count`; that a state a page cannot reach is silently
    skipped (visible only under `--verbose`); and that the CJK face list is a
    constant, so a face shipped by a future plugin and not added to it reads as
    an undeclared font.
  - **What was NOT promoted, and why** — the paragraph below, in the README's own
    words.

TIPCHECK AND TRACE — the recommendation, to be written into the README's
"What was NOT promoted" section and repeated in the SUMMARY.

`tipcheck.js` is **not promoted here**, and not folded in as `--report tips`.
Three reasons. Its unit is a TIP BINDING read from the plugin's `TIP_BINDINGS`
export, not a measured DOM node, so it does not share this file's row schema and
folding it in would make `--from` mean two different things. `boot-all-uis
--strict-tips` already re-queries every missed selector after settle and
distinguishes LATE from DEAD, so the only thing tipcheck adds over a committed
tool is the PER-LANGUAGE arm. And it carries a fixed 1200x900 viewport — the
exact defect C3 names — so promoting it as written would commit the mistake this
task exists to prevent. Its real home is the tip-render gate D6 says
O-IntonationPad lacks, which is a separate pass with its own budget; record it
there rather than half-landing it here.

`trace.js` is **not promoted**. It is a one-off console-stack tracer built to
find O-IntonationPad's late binding: it hardcodes the warning substring it
watches for, takes the default viewport, and has no output contract. There is
nothing in it to reuse that `boot-all-uis --verbose` does not already give.

SMOKE VERIFICATION — run the promoted tool against both plugins and record the
counts in the SUMMARY:
  - **O-IntonationPad** (19 states, 800x500 — the plugin that produced the
    fourth carrier form)
  - **O-Emulator** (620x430 — the wrap-count plugin)
Record for each: languages walked, rows measured, distinct display ids, and the
count from each of the four screens. Expect the three repair screens to read 0
on both; state that expectation and then state the control result beside it, so
the SUMMARY shows a 0 that was earned.

COMMIT — path-scoped, on `main`, in a shared checkout. Run
`git branch --show-current` and `git status --short` IMMEDIATELY before the
commit, not once at task start: another session's staging can join yours in the
gap. Never `git add -A`, never `git commit -a`. Stage nothing under
`plugins/O-Orbit/libs/SAF`. Then:

    git commit -m "feat(quick-260905-izp): promote the wave-4b measurement harness to scripts/measure-ui.js

    Closes D7. Computed-style census at the parsed shipping frame, cumulative
    state walk, visibility OR-ed across states, one row per DOM node, and the
    four wave-4b defect screens as --report filters over the same measurement.

    Claude-Session: https://claude.ai/code/session_01Ay4bv3DT3RrV7eprJVz1aD" \
      -- scripts/measure-ui.js scripts/measure-ui-README.md
  </action>
  <verify>
    <automated>node scripts/measure-ui.js --plugin O-IntonationPad --mode fonts --report all > /tmp/izp-ip.json 2>/tmp/izp-ip-report.txt; echo "exit=$?"; cat /tmp/izp-ip-report.txt; node -e "const r=require('/tmp/izp-ip.json'); const L=[...new Set(r.map(x=>x.lang))]; console.log('languages:',L.join(','),'rows:',r.length,'distinct display ids:',new Set(r.map(x=>x.id)).size); if(r.length===0){console.error('FAIL: nothing measured on the 19-state plugin');process.exit(1)}"</automated>
    <automated>node scripts/measure-ui.js --plugin O-Emulator --mode box --report all > /tmp/izp-emu2.json 2>/tmp/izp-emu2-report.txt; echo "exit=$?"; cat /tmp/izp-emu2-report.txt; grep -qE '^[[:space:]]*undeclared-font: 0 finding' /tmp/izp-emu2-report.txt && echo "OK: undeclared-font reads 0 on a repaired plugin, as expected"</automated>
    <automated>node scripts/measure-ui.js --plugin O-Emulator --mode fonts --select button 2>/dev/null | node -e "let s='';process.stdin.on('data',d=>s+=d).on('end',()=>{const r=JSON.parse(s);const z=r.filter(x=>x.lang==='zh-Hans');console.log('POSITIVE CONTROL — button stacks under zh-Hans:');z.forEach(x=>console.log('  ',x.id,'|',x.ff));if(z.length===0){console.error('FAIL: control measured no buttons — a 0 from undeclared-font would be vacuous');process.exit(1)}const tailed=z.filter(x=>/PingFang SC|Microsoft YaHei|Songti SC/.test(x.ff||''));if(tailed.length===0){console.error('FAIL: no button names a CJK face — either the repair regressed or the screen is blind');process.exit(1)}console.log('OK: '+tailed.length+'/'+z.length+' buttons name a CJK face, so the 0 above is a measured 0')})"</automated>
    <automated>node -e "const s=require('fs').readFileSync('scripts/measure-ui-README.md','utf8'); const need=['620','77','cumulative','24','tipcheck','trace']; const miss=need.filter(k=>!new RegExp(k,'i').test(s)); if(miss.length){console.error('FAIL: README missing '+miss.join(', '));process.exit(1)} console.log('OK: README carries the parsed frame, the exit contract, the state-walk lesson, the identity evidence, and both not-promoted decisions')"</automated>
    <automated>git branch --show-current; git status --short; git log --oneline -1 -- scripts/measure-ui.js scripts/measure-ui-README.md; echo "--- files in HEAD ---"; git show --name-only --format= HEAD | sed '/^$/d' | tee /tmp/izp-head-files.txt; test "$(sort /tmp/izp-head-files.txt | tr '\n' ' ')" = "scripts/measure-ui-README.md scripts/measure-ui.js " && echo "OK: commit is path-scoped to exactly the two new files" || { echo "FAIL: commit touched files outside scripts/measure-ui*"; exit 1; }</automated>
  </verify>
  <done>`scripts/measure-ui-README.md` exists in the style of `check-ui-labels-README.md`, carrying the exit table, the three method lessons with their measured evidence, the identity evidence, the four screens, how to read a 0, the known limitations, and the reasoned decisions not to promote `tipcheck.js` or `trace.js`. Both plugins measured, counts recorded in the SUMMARY beside a positive control that reports non-zero. One path-scoped commit on `main` touching only the two new files, message ending in the session line.</done>
</task>

</tasks>

<verification>
1. `node scripts/measure-ui.js` alone prints a usage line and exits 2.
2. The file contains no path beginning with a user home directory.
3. O-AnalogEQ's 24 sibling SVG text nodes report as 24 rows, not 1 — the single
   sharpest proof the promotion is not a copy of the scratch file.
4. All four screens print a count from a saved JSON via `--from`, with no
   browser launched.
5. `undeclared-font` reads 0 on O-Emulator AND the `--select button` control
   reports buttons naming a CJK face, so the 0 is a measured result rather than
   a dead screen.
6. README carries the parsed 620x430, the discrepancy note, and both
   not-promoted decisions.
7. `git show --stat HEAD` lists exactly `scripts/measure-ui.js` and
   `scripts/measure-ui-README.md`.
</verification>

<success_criteria>
- D7 is closed: the harness is in `scripts/`, runs from a portable root, and
  carries its three method lessons in its own header so the next wave cannot
  rebuild it wrong without first being told.
- The analysis half is promoted too, as four pure-function screens re-runnable
  against a saved measurement.
- Node identity is per-node, and the disclosure line makes the change from
  wave-4b counts legible rather than mysterious.
- No plugin changed, no existing gate edited, `trace.js` not promoted, and the
  `tipcheck.js` decision written down with its reasoning rather than left
  implicit.
</success_criteria>

<output>
Create `.planning/quick/260905-izp-promote-the-wave-4b-measurement-harness-/260905-izp-SUMMARY.md` when done.
Include: the two plugins' measured counts (languages, rows, distinct ids, four
screen counts each), the positive-control output, the O-Emulator frame
correction, and the tipcheck/trace decisions.
</output>
