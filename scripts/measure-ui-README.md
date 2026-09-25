# `scripts/measure-ui.js` — the per-node computed-style census

This is what retires **D7** from wave 4b: *"the measurement harness lives in
scratch, not in the repo."*

Four defect classes in that wave were findable **only** by computed style, in
every language, across every state, at the frame the plugin actually ships. The
harness that found them was written for the wave and thrown away. Every wave
that needs it rebuilds it — and wave 4b proved a rebuild can be **wrong** in
ways that hide real defects instead of announcing themselves. It rebuilt it
wrong twice.

```bash
node scripts/measure-ui.js --plugin O-Emulator                       # fonts mode
node scripts/measure-ui.js --plugin O-Emulator --mode box --report all
node scripts/measure-ui.js --plugin O-AnalogEQ --select text --report svg-font-attr
node scripts/measure-ui.js --report wrap-count --from /tmp/rows.json  # no browser
node scripts/measure-ui.js --plugin O-Fixture --root /tmp/fix --verbose
node scripts/measure-ui.js --plugin O-Prism --contrast               # opt-in WCAG AA report
```

Raw JSON goes to **stdout** and nothing else does, so this composes with `jq`
and node one-liners. Diagnostics, the identity disclosure and every screen count
go to **stderr**.

| Exit | Meaning |
|---|---|
| `0` | The run completed. Read stdout. |
| `1` | The harness itself could not run: no such plugin, no `setSize` in `PluginEditor.cpp`, no `js/i18n.js`, no `LANGUAGES` export. |
| `2` | Usage error, including `--report` with an unknown screen name. |
| `77` | Playwright unresolvable — **nothing was measured**. Never a pass. |

**A screen finding defects is a `0`.** This is a report, not a gate, and the
name says so: `check-` in this repo means a gate (`check-ui-labels` exits `n>0`
on `n` failures), `measure-` does not. The reasoning is `boot-all-uis.js`'s: a
permanently-red gate on findings no wave has budgeted to fix destroys the habit
of reading gates, and these findings are read by a person deciding what a
localization wave should repair, not by CI.

## The frame is parsed, never chosen

The viewport comes from `readEditorSize()`, which reads `setSize(W, H)` out of
that plugin's own `Source/PluginEditor.cpp` — the same source `check-ui-labels`
parses. A wider frame measures a page nobody ships: a caption that wraps to
three lines at the shipped frame fits on one at 1200px, and the whole class of
wrap-count defects simply is not there to be found.

**O-Emulator's parsed frame is `620 x 430`.** Wave 4b's SUMMARY and its
`deferred-items.md` both name "700 x 380" for that plugin. The value parsed from
`plugins/O-Emulator/Source/PluginEditor.cpp` disagrees, and the parsed value is
the one this tool uses. The **lesson is unaffected** — a wide viewport hides
wrap-count defects at either number — but the number quoted alongside it in
those documents is wrong. They are a historical record and are left alone; this
is the correction.

What makes the frame fragile is a silent fallback, so the mechanism is worth
naming. Playwright accepts `viewport`. It **silently ignores** the misspelled
plural `viewportSize` — that is the getter's name, not a launch option — and
leaves Chromium's 1280x720 default. One wrong identifier turns every measurement
in the file into a measurement of a page that does not exist, and nothing
reports it.

## The state walk is cumulative

The page loads **once per language** and the states in
`plugins/<Name>/tests/i18n-states.json` apply in file order **with no reset**,
because a state can require an earlier one: O-IntonationPad's rotation view is
reachable only through the tuning tab. Resetting between states measures pages
those states never produce, and every panel behind a second click goes
unmeasured.

Three step forms are accepted — the three `check-ui-labels.js` documents and the
three the 43 shipped state files use. Their real distribution, counted across
all 43:

| key | occurrences |
|---|---|
| `name` | 229 |
| `click` | 169 |
| `eval` | 60 |
| `dblclick` | **1** (O-Octagon) |

The scratch original handled `click` / `select` / `eval`. `select` appears in
**no** state file in the repo, and `dblclick` appears in exactly one — which is
precisely why handling the wrong three looked correct: a `dblclick` entry
produced an empty step list, the state was a silent no-op, and O-Octagon's
popover was measured as if it had never opened. Under `--verbose` a step that
does not resolve is now named on stderr.

## Visibility and Han are OR-ed across states (W2)

Keying a node on first sighting while walking cumulatively makes every node read
as **invisible** for any panel the walk later navigates away from. On O-Prism
that was **19** visible Han-bearing nodes found where **48** existed. So `vis`
and `han` are OR-ed over the whole walk; the remaining fields are taken from the
last state in which the node was actually visible.

## Identity is a DOM path, not a tag-and-class string

The largest defect found in the scratch original, and the one that changes how
wave-4b numbers should be read. It keyed its accumulator on
`lang + ' ' + (#id | tag.class.class)`, so every node sharing a shape collapsed
into **one** row and the last visible one won.

Measured, by running this tool over the same pages:

| plugin | rows | distinct DOM keys | distinct display ids |
|---|---|---|---|
| O-AnalogEQ (`--select text`) | 87 | **29** | **1** |
| O-Emulator | 210 | **70** | **47** |
| O-IntonationPad | 2844 | **948** | **318** |

O-AnalogEQ's 29 sibling SVG `<text>` nodes share the display id `text` and
therefore reported as a **single row**. O-Emulator's whole page reported as 47.
O-IntonationPad's as 318, against 948 nodes actually on the page.

**So every per-node count wave 4b produced was a lower bound**, not a census,
and any screen run over such a key is structurally blind to siblings. Every run
of this tool prints the ratio to stderr —

```
identity: 2844 nodes / 948 distinct DOM keys / 318 distinct display ids — keyed by DOM path
```

— so a reader comparing a number here against a wave-4b number can see why it
moved instead of treating the difference as a mystery.

## The four screens

Every screen is a **pure function** over the row array: no Playwright, no
filesystem, no network. That is what makes `--from` work, and it is the half of
D7 that promoting the measurement alone would not have fixed — wave 4b rebuilt
the *analysis* by hand too.

**Every screen prints its count, including when it is `0`.** A screen that
prints nothing on a repaired plugin is indistinguishable from a screen that did
not run. A screen whose fields are absent prints `SKIPPED` and **no** count, so
it cannot be mistaken for a screen that ran clean:

```
undeclared-font: 0 finding(s)
  faces checked against: PingFang SC, Microsoft YaHei, Songti SC
line-height-normal: SKIPPED — needs --mode box (field lh not present)
```

### `undeclared-font`

Visible nodes carrying Han in any carrier (own text, `data-tip`,
`data-tip-title`, `aria-label`) whose computed `font-family` names **no** CJK
face. Runs in either mode.

This is **form 4** of the font-carrier census (wave 4b, C2). `<button>`,
`<select>` and `<input>` do **not inherit `font-family`** — the UA stylesheet
gives them one, on this build Arial — so a sweep that reads every CSS
declaration *and* every SVG attribute in both files still misses them. Wave 4b
found 13 on O-IntonationPad and 1 on O-AnalogEQ this way, and no grep in this
repo can see the class at all.

The face list is a **named constant** and is printed on every run, so a clean
result cannot be confused with a result produced by an empty list. It is seeded
from the faces actually shipped here — `PingFang SC` (147 declarations),
`Microsoft YaHei` (137), `Songti SC` (3) — and `--cjk-faces a,b` extends it.

### `line-height-normal`

Visible Han-carrying **leaf** nodes whose computed `line-height` resolves to
`normal`. Restricted to leaves for the reason `check-ui-labels` restricts its
clip check to them: a container's box does not report a text metric.

### `wrap-count`

Per node, `lines = round((h - pt - pb - bt - bb) / lineHeightPx)`, reported
wherever the count **differs** between English and any non-English language.
Needs `--mode box`. This is what caught O-Emulator's engraved plate caption in
wave 4b — three lines in en/fr, two in zh — a defect that does not exist at
1200px wide.

`lineHeightPx` is the computed value, or `fsn * 1.2` when it resolves to
`normal`. **That 1.2 is an approximation of the UA's normal line box, not a
measurement.** A node the `line-height-normal` screen also catches is therefore
a node whose wrap count is *estimated* rather than measured; the two screens are
related and should be read together. Each run prints how many of the compared
nodes were estimated.

### `svg-font-attr`

Two counts, both printed:

- **(a) attribute carriers** — every node carrying a `font-family` presentation
  attribute. This is the screen's own liveness signal.
- **(b) findings** — the subset whose attribute **diverges** from the computed
  stack.

(b) is the useful half, and the reason this is a divergence report rather than a
presence report: a presentation attribute has the **lowest specificity** of any
`font-family` source, so a CSS rule overrides it silently. Someone reading only
the markup and someone reading only the computed style reach different
conclusions, and neither learns that the two disagree.

**A correction to wave 4b's account, measured here.** On O-AnalogEQ the screen
reports `24 attribute carrier(s), 0 finding(s)`. The 24 attributes read
`Garamond, 'Times New Roman', serif` and the computed stack on those same nodes
reads exactly that — **the CJK tail is not there**. The tail appears only on the
**5** `<text>` nodes that carry *no* attribute, whose computed stack is
`Garamond, "Times New Roman", "PingFang SC", "Microsoft YaHei", serif`. No CSS
rule targets the 24, so the lowest-specificity source is the only source, and
the attribute is what keeps the tail off them. Those 24 are numeric tick labels
and carry no Han, so this is a finding to **read**, not a defect — but it is the
opposite way round from the account wave 4b left behind.

## The contrast report (opt-in)

R4 of the UI design review (quick task 260924-nho, §2.2) found that the
naturalist template's own text colours fail WCAG AA at the sizes it prescribes,
and that nothing in the repo measured contrast. `--contrast` is that
measurement, using the same method as the review's A.4 table so the numbers
stay comparable.

```bash
node scripts/measure-ui.js --plugin O-Prism --contrast          # or: --report contrast
```

**What is measured.** Every visible element that owns a non-empty text node
gets a `ct` field in its row:

- `fg` — its own computed `color` (SVG: computed `fill`), alpha multiplied by
  the computed `opacity` of the node and every ancestor, composited over `bg`.
- `bg` — the nearest ancestor-or-self whose `background-color` alpha is
  `>= 0.99`, with every semi-transparent `background-color` on the way
  composited on top, outermost first. When nothing on the chain is opaque the
  base is `#FFFFFF`, Chromium's default canvas colour.
- `ratio` — WCAG 2.x, computed from the 8-bit rounded `fg`/`bg`, so a
  recompute from the two hex strings reproduces it exactly.
- `need` (`4.5`, or `3` for large text: `>= 24px`, or `>= 18.66px` at weight
  `>= 700`), `large`, `fs`, `fw`, `img` (a `background-image` somewhere up to the
  base), `ah` (under `aria-hidden="true"`), and `skip` (`'transparent'` below
  0.01 effective alpha, `'unparsed'` for a colour neither the regex nor the
  1×1 canvas fallback can read).

**Output.** The count line is `contrast: <N> finding(s)`, where N is the counted
rows that are below AA **or** under the 9px text floor. Then, per language:

```
  en: <T> text node(s), <B> below AA (<pct>%), <F> under 9px floor, <I> over background-image, ratio median <m> / min <n>
```

followed by the aria-hidden count, the skipped counts, the thresholds and the
background method, then one finding per node sorted by ratio ascending and
tagged `[AA]`, `[<9px]`, `[img]`, `[aria-hidden]`. Report-only: findings never
change the exit code.

**Why it is opt-in, and outside `all`.** `ct` is collected only when asked for.
A run without `--contrast` is byte-identical to a run before this report
existed (verified on O-TextureForge in fonts mode and `--mode box --report all`),
and `--report all` still means the four screens above. Putting `contrast` in
`all` would add a SKIPPED line to every existing `all` run. Over rows saved
without `ct`, the report prints `contrast: SKIPPED — needs --contrast (field ct
not present)`.

**Reference reading** — `en`, at `b2f3d7ba`, measured on a `git archive`
snapshot (not the working tree), every state × every language. `fr` and
`zh-Hans` read identically on all four.

| Plugin | Frame | Text nodes | Below AA | Under 9px | Over bg-image | Median / min |
|---|---|---|---|---|---|---|
| O-TextureForge | 900×600 | 35 | 24 (68.6%) | 10 | 28 | 4.49 / 1.46 |
| O-MicrotonalSampler | 900×640 | 632 | 106 (16.8%) | 337 | 528 | 10.38 / 2.02 |
| O-Prism | 1200×800 | 652 | 230 (35.3%) | 55 | 1 | 7.44 / 1.36 |
| O-ReverseDelay | 940×693 | 89 | 8 (9.0%) | 0 | 73 | 7.67 / 1.78 |

Read the `Over bg-image` column beside the rest. On O-TextureForge, 17 of the
24 below-AA nodes are walnut `#8B7355` over the paper JPG with no opaque
`background-color` anywhere on the chain, so they are measured against the
`#FFFFFF` canvas fallback at **4.49**. Against a `#F5E6D3`-toned paper the same
walnut reads 3.66, so the true count there is a lower bound.

## Reading a `0`

Three of the four screens read `0` on the plugins wave 4b already repaired.
A `0` is only worth anything beside a control that reports non-zero on the same
data. The first two are cheap; the third covers the contrast report:

**Control 1 — the carrier the screen exists to see.** `undeclared-font` reads 0
on O-Emulator. This proves the input was not empty and the tail is really there:

```bash
node scripts/measure-ui.js --plugin O-Emulator --mode fonts --select button
```

```
POSITIVE CONTROL — button stacks under zh-Hans:
   #preset-prev | "EB Garamond", Garamond, "Adobe Garamond Pro", "Times New Roman", "PingFang SC", "Microsoft YaHei", serif
   ... 12 rows ...
OK: 12/12 buttons name a CJK face, so the 0 above is a measured 0
```

An independent scan of the same rows is the other half — on O-Emulator,
`rows.filter(x => x.vis && x.han).length` is **21**, so the screens had
non-empty input and a `0` is a result rather than a vacuum.

**Control 2 — the divergence arm of `svg-font-attr`.** Its finding count is 0 on
every plugin measured so far, which is exactly the shape of a dead screen. Fire
it over a saved measurement with one attribute mutated:

```bash
node -e "const r=require('/tmp/izp-aeq.json'); const i=r.findIndex(x=>x.ffAttr);
         r[i]=Object.assign({},r[i],{ffAttr:'Helvetica, sans-serif'});
         require('fs').writeFileSync('/tmp/mutated.json',JSON.stringify(r))"
node scripts/measure-ui.js --report svg-font-attr --from /tmp/mutated.json 2>&1 1>/dev/null
```

```
svg-font-attr: 24 attribute carrier(s), 1 finding(s)
    html/.../svg[1]/text[1]  (text)  attr=[Helvetica, sans-serif]  computed=[Garamond, "Times New Roman", serif]
```

Because the screens are pure functions, this control needs no browser and no
plugin edit.

**Control 3 — the contrast fixture.** A contrast `0` is only readable beside
this control, because a report whose compositing is wrong reads clean just as
easily as one whose page is clean. Build a scratch tree (never inside the repo —
a committed `O-*` folder is picked up by repo-wide globs):

- `<root>/modules/`, empty;
- `<root>/plugins/O-Fixture/CMakeLists.txt` — `juce_add_binary_data(O-Fixture_WebUI SOURCES`
  listing `Source/ui/public/index.html` and `Source/ui/public/js/i18n.js`;
- `<root>/plugins/O-Fixture/Source/PluginEditor.cpp` — `setSize (400, 300);`;
- `<root>/plugins/O-Fixture/Source/ui/public/js/i18n.js` — `export const LANGUAGES = ['en'];`;
- `<root>/plugins/O-Fixture/Source/ui/public/index.html` — body `margin:0`,
  background `#F5E6D3`, font-size 10px, `p { margin: 0 }`, and these elements,
  each with a short text:

  | id | Markup | Expected |
  |---|---|---|
  | a | `p`, `#8B7355` | 3.66, need 4.5, below |
  | b | `p`, `#7A654B` | 4.52, pass |
  | c | `p` in a div with background `#EBD9C7`, colour `#715D45` | 4.56, pass |
  | d | `p`, `#55703E` | 4.54, pass |
  | e | `p`, `#8B7355` at 26px | 3.66, need 3, pass (large) |
  | f | `p`, `#6B8E4E` at 19px weight 700 | 3.06, need 3, pass (large bold) |
  | g | `p`, `#6B8E4E` at 19px weight 400 | 3.06, need 4.5, below |
  | h | `p`, `#3C2F2F` at 8px | 10.45, pass, under the floor |
  | i | `p`, `#000000`, in a div `rgba(0,0,0,0.5)` in a div `#FFFFFF` | bg `#808080`, 5.32 |
  | j | `p`, colour `rgba(60,47,47,0.5)` | fg `#998B81`, 2.69, below |
  | k | `p`, `#3C2F2F`, in a div with `opacity: 0.5` | 2.69, below |
  | m | `p`, `#3C2F2F`, in a div with background-color `#F5E6D3` and `background-image: linear-gradient(#000,#000)` | 10.45, `img` |
  | n | `span aria-hidden="true"`, `#8B7355`, text `&#10086;` | 3.66, below, `ah` |
  | o | `p`, `#3C2F2F`, `opacity: 0` | skipped `transparent` |
  | p | `p`, `display:none` | not counted |
  | q | `p`, `visibility:hidden` | not counted |

```bash
node scripts/measure-ui.js --plugin O-Fixture --root <root> --contrast 2>&1 1>/dev/null
```

```
contrast: 6 finding(s)
  en: 13 text node(s), 5 below AA (38.5%), 1 under 9px floor, 1 over background-image, ratio median 3.66 / min 2.69
  1 of the below-AA node(s) sit under aria-hidden (decorative) — still counted
  skipped: 1 transparent, 0 unparsed colour
```

Those two lines exercise layer compositing (i), the opacity product (j, k), the
large-text rule (e, f against g), the floor (h), the image flag (m) and the
transparent skip (o) against hand-computed values.

## Known limitations, carried openly

- **The `1.2` normal line box in `wrap-count` is an approximation.** Every node
  whose `line-height` computes to `normal` has an *estimated* line count, not a
  measured one. The count of such nodes is printed on every run
  (O-IntonationPad: 126 of 368 compared; O-Emulator: 14 of 29).
- **A state the page cannot reach is silently skipped.** A step whose selector
  does not resolve is a state this page does not have, and the walk continues —
  correct behaviour, but it means an unreachable state is a coverage hole rather
  than an error. It is named on stderr only under `--verbose`. Run with
  `--verbose` before trusting a low count.
- **The CJK face list is a constant.** A face shipped by a future plugin and not
  added to `CJK_FACES` (or passed via `--cjk-faces`) reads as an undeclared
  font. The list is printed on every run so this is visible rather than silent.
- **`wrap-count` compares text-bearing leaf nodes only** — visible, no element
  children, non-empty own text. A caption whose text is split across child
  spans is not compared.
- **`--select` narrows the sweep, and a narrowed sweep is not a census.** The
  identity disclosure counts what was measured, not what is on the page.
- **Han detection is a code-point test** (`U+3400–U+4DBF`, `U+4E00–U+9FFF`) over
  four carriers. A Han string reaching the page by any other route — canvas,
  a pseudo-element's `content`, an SVG `<title>` — is invisible to every screen
  here.
- **`contrast` does not sample `background-image`.** A paper JPG, gradient or
  botanical plate is flagged (`img`, `[img]`), never read. When no opaque
  `background-color` sits on the chain the base is the `#FFFFFF` canvas
  fallback, which can make a node read higher than it paints.
- **`contrast` walks ancestors only.** An absolutely positioned overlay or a
  stacked sibling painted behind the text is invisible to it.
- **`contrast` counts own text nodes only.** Text in a pseudo-element's
  `content`, an input's value and a closed `<select>`'s caption is not counted.
- **`contrast` reads the CSS font size.** It ignores CSS transforms, `zoom` and
  SVG viewBox scaling, so the large-text rule and the 9px floor judge the
  declared size, not the painted size.
- **`contrast` ignores `filter: opacity()`, and applies `opacity` to the
  foreground only**, not to the background layers.
- **`contrast` takes each node's values from the last state in which it was
  visible**, like every other field (design note 3).

## What was NOT promoted, and why

Two other files were written in the same scratchpad during wave 4b. Neither is
promoted, and the reasoning is recorded here rather than left implicit so the
next wave does not re-litigate it from scratch.

### `tipcheck.js` — not promoted, and deliberately not folded in as `--report tips`

Three reasons.

1. **Its unit is a tip binding, not a measured DOM node.** It reads
   `TIP_BINDINGS` out of the plugin's own `i18n.js` and asks whether each anchor
   carries `data-tip` and `data-tip-title` at settle. That does not share this
   file's row schema, and folding it in would make `--from` mean two different
   things depending on which screen you ran.
2. **`boot-all-uis --strict-tips` already covers most of it.** It re-queries
   every missed selector after settle and distinguishes **LATE** from **DEAD**.
   The only thing `tipcheck.js` adds over a tool that is already committed is
   the **per-language** arm.
3. **It carries a fixed `1200 x 900` viewport** — the exact defect wave 4b's C3
   names. Promoting it as written would commit the mistake this task exists to
   prevent.

Its real home is the tip-render gate that **D6** says O-IntonationPad lacks —
a separate pass with its own budget. Record it there rather than half-landing it
here.

### `trace.js` — not promoted

A one-off console-stack tracer built to find O-IntonationPad's late tip binding.
It hardcodes the warning substring it watches for (`'tip target'`), takes
Chromium's default viewport, and has no output contract at all. There is nothing
in it to reuse that `boot-all-uis --verbose` does not already give.
