---
task: 1
plan: 260906-h8y
role: TRACER
plugin: O-Tapestop
status: complete
shipped_version: "1.7.0"
plugin_type: effect
au_triple: "aufx OTsp OuDv"
rows: 114
corpus_before: 3073
corpus_after: 3187
plugins_before: 27
plugins_after: 28
commits: [1ad239b4, 04a7561a]
---

# Task 1 — O-Tapestop 1.7.0, the wave-4e tracer

**Shipped:** O-Tapestop **1.7.0**, type **effect** (`IS_SYNTH` absent — the wave's only
non-instrument). AU triple read off `auval -a`: **`aufx OTsp OuDv`** — already registered as
`Ouaricon Audio Development: O-Tapestop-dev`. **`auval -v` NOT run** — deferred to Task 3's single
cold sweep, per the plan.

**Corpus:** 3073 → **3187** rows of 5256, all at `reviewed: 'bt'`, `BELOW SHIP BAR 0`.
27 → **28** plugins carrying Chinese. 114 rows = 35 `I18N` entries × 2 (title + body) + 44 `LABELS`.

`build-and-install.sh O-Tapestop` exit 0 in 48 s; VST3 + AU installed as `O-Tapestop-dev`;
**no alternate-variant orphan on disk** (CLAUDE.md's dual-variant sweep — only the `-dev` pair
exists in both plug-in folders).

---

## Gate results

Every one run in the background and polled. All fired on the tree as found BEFORE any edit
(M3 / instruction 3) and again on the shipped tree.

| gate | as found | shipped | note |
|---|---|---|---|
| `i18n-zh-lint --self-test` | **10/10** | **10/10** | no zero trusted without it |
| `check-i18n` repo-wide | exit 0, 43 plugins | **exit 0, 43 plugins** | `LANGUAGES` reads three |
| `i18n-zh-lint` repo-wide | exit 0, 0 findings | **exit 0, 0 findings across 43** | gate mode (exit 2) |
| `i18n-zh-lint --plugin O-Tapestop` | — | **exit 0, 0 findings, 79 entries / 114 rows** | 2 `termNote` exemptions |
| `i18n-fr-lint` | exit 0 | **exit 0** | French untouched |
| `check-ui-labels --plugin O-Tapestop` | exit 0 | **exit 0 — 0 moved on en, fr AND zh, all 6 states** | |
| `ui_tooltip_clamp_check.js` | **exit 0, `== ALL CHECKS PASSED ==`** (FIRED, not assumed) | **exit 0, 154 PASS, real zh arm swept AND compared** | |
| `boot-all-uis --plugin O-Tapestop --strict-tips` | — | **exit 0, 0 late, 0 dead** | |
| `boot-all-uis --strict-tips` repo-wide | — | **0 dead, 0 failed, 0 warn, exactly 2 late across 1 plugin** | D2 census control unchanged (O-Bells) |
| `measure-ui --verbose` | — | **0 unresolved states** | every state-gated zero is measured |
| `git tag --points-at HEAD` | 0 | **0** | no tag created |

**Clamp gate, shipped, three languages:**

| lang | anchors | clamped | flipped-below | widest | tallest |
|---|---|---|---|---|---|
| en | 35 | 13 | 4 | 230.0 | 104.2 |
| fr | 35 | 13 | 4 | 230.0 | 119.1 |
| **zh-Hans** | 35 | 13 | **5** | 230.0 | **92.4** |

Chinese costs +1 vertical flip and is 11.8 px **shorter** at its tallest — reported, never required.
`PASS: [zh-Hans] every anchor's copy actually CHANGED between en and zh-Hans — 35/35 differ` is the
line that only exists because the hard-fail was generalized.

---

## measure-ui, before and after the font work

`--report all`, at the parsed shipping frame 860 × 580, cumulative over 6 states.
**Identity disclosure: 630 nodes / 210 distinct DOM keys / 120 distinct display ids — keyed by DOM
path** (non-zero, so the zeros below are measured rather than vacuous).

| screen | before | after | |
|---|---|---|---|
| `undeclared-font` | **77** | **0** | against **77 visible Han-bearing nodes** on the same rows |
| `line-height-normal` | 46 | **4** | 3 text-bearing residuals + 1 empty `<canvas>`, **0 of which MOVED** |
| `wrap-count` | 2 | **2** | both evidenced non-movers — see below |
| `svg-font-attr` | 0 carriers, 0 | 0 carriers, 0 | form 2 of the font problem is absent |
| en+fr nodes compared before vs after the tail edit | — | **420, 0 MOVED** | M7 verified by re-running the screen |

**`line-height-normal` residuals, each named with its measured `enH == zhH` (N1):**

| node | font-size | enH | zhH | why it is a permanent residual |
|---|---|---|---|---|
| `#help-toggle` | 10px | **21** | **21** | fixed box; renders `开`/`关`, one Han glyph in a sized button |
| `#preset-name` | 13px | **30** | **30** | renders `Default` — an `I18N_EXEMPT` Latin string; **no Han to render** |
| `#engage-btn` | 15px | **58** | **58** | fixed-height performance button |
| `#envCanvas` | 16px | — | — | a `<canvas>`; empty own text, no line box at all |

N1 holds exactly as written: the screen **cannot** reach 0 on this page, the load-bearing criterion
is `check-ui-labels` reporting 0 moved, and it does on all three arms across all six states.

**`wrap-count`, both findings adjudicated, neither fixed:**

1. `label.envHint1` — **en = 1 line, fr = 2 lines.** A **pre-existing French wrap**, present before
   this work and untouched by it. Nothing to do with Chinese.
2. `label.envHint2` — **en = 2 lines, zh = 1 line.** Chinese is SHORTER (N9's shape, a shrink not a
   clip). Both hints sit inside `.env-plate`, whose own comment records that it "holds four lines at
   this line-height; the content centres in it" — a fixed-height plate. `check-ui-labels` reports
   **0 moved on all three arms in all six states**, so neither wrap difference displaces anything.

---

## The gate repair — the six-site classification, recorded for reuse

`plugins/O-Tapestop/tests/ui_tooltip_clamp_check.js`, 789 → 830 lines.

**Q2 confirmed exactly.** The file already required `vm` (L86), already read `js/i18n.js`, already
ran it in a context (L201) — and published `{ I18N, TIP_BINDINGS }`. Measured as found:
**`grep -c LANGUAGES` = 0, `grep -c vm` = 1.** Only the first number matters, and only one identifier
was missing. Shipped: **`LANGUAGES` = 4 references.**

**The six sites, by RAW line number as found, with what was done and why:**

| raw line | form | what it fed | disposition |
|---|---|---|---|
| L201 | — | `runInContext` published set | **`LANGUAGES` added** — one identifier |
| **L455** | **3** — two-element array literal | `const LANGS = ['en','fr']` | **DERIVED** from `i18nBox.__x.LANGUAGES`, shape asserted, **ABORT** if unreadable, no default pair |
| L459 | 2 — call-site walk | the language sweep | free, follows the derived list |
| **L639/L640** | **5** — per-language map lookup | **the REAL hard-fail** at L645 `check(same.length === 0, …)` | **GENERALIZED** to `for (const lang of LANGS.filter(l => l !== 'en'))`. Left alone it would have compared exactly two languages while the Chinese page was driven, measured and **never compared** |
| L654 | 2 — call-site walk | per-language geometry print | free |
| **L661** | **5** — per-language map lookup | a `console.log` only | **GENERALIZED**, following O-ReverseDelay's shipped precedent (its L624–626), with the reason at the site: an unexamined print and an unexamined assertion look identical in a diff |
| **L671** | *not* an enumeration | `__setLanguage('en')` | **LEFT — a deliberate RESET.** Its own comment says the stress and layout stages below must stay comparable with every earlier release's numbers |
| **L673** | *not* an enumeration | `(stats.get('en')||{}).worstRight` | **LEFT — reads the language just restored** |
| **L674** | *not* an enumeration | `.worstRightLabel` | **LEFT — same read, same figure** |

All three reset-scoped sites carry a `RESET-SCOPED (see above)` comment at the line, so the next
reader does not conclude they were missed.

**Derive-or-abort control FIRED against the WALK.** A planted `export const LANGUAGES = [];` made
the gate print
`FAIL: LANGUAGES parsed from js/i18n.js and drives this sweep — got []` followed by
`ABORT: LANGUAGES is unreadable … Refusing to sweep a guessed language list.` and exit **1**, with
**zero languages swept** — the walk never ran, which is the point of firing it against the walk
rather than only against the guard. Reverted by **targeted edit, never `git checkout --`**;
`shasum -a 256 -c` reported **OK — byte-identical** to the pre-plant file.

**Port source used:** `plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js` at HEAD, line for
line at Q3's cited lines. Every one was accurate.

---

## Blind reverse read

**Shape, proven here for Tasks 2 and 3.** Two `--emit O-Tapestop --plugin O-Tapestop` runs (both
flags — W4), each with `--forward-provenance`, each producing its **own 16-hex salt**
(`678c71dc…` and `a525900328…`, confirmed different). Because the emitter sorts by blinded id, the
two emits carry **different row orders**, so "first 57 of A / last 57 of B" would have overlapped
and missed rows. The chunks were therefore built by resolving chunk A's real ids through manifest A
and taking the **complement** through manifest B: **57 + 57, disjoint, covering all 114 exactly
once.** Dispatched with `claude -p … --model sonnet --allowed-tools ""` from `/tmp/h8y-blind`, a
cwd **outside** the repo, with repo/file/web access forbidden in the prompt.

**Controls, all fired and observed:**

| control | result |
|---|---|
| ids pure 12-hex | **0 non-conforming** |
| ids carrying a key fragment | **0** |
| **M12 — ids identical AND IN ORDER, checked BEFORE ingest** | **57/57 and 57/57, `diff` clean on both chunks** |
| Han surviving in the returned English | **0 on both chunks** |
| malformed returned lines | **0 on both chunks** |
| **refusal 1 — emit without `--forward-provenance`** | **FIRED:** `REFUSED: the batch was emitted with no recorded forward pass — the identity check cannot run` |
| **refusal 2 — wrong-side `--manifest`** | **FIRED:** `joined: 0   unjoinable ids: 3` … `WRONG MANIFEST FOR THIS BATCH: all 3 returned ids are unjoinable, not one.` |
| product-name-in-batch | **0 hits.** Reported, not treated as pass/fail: this plugin's own copy does not name itself, unlike the waves where this control reads non-zero |
| Latin surviving in the zh column | exactly **`MIDI`** (`SAME_AS_EN`) and **`alt`** (a keyboard modifier) — both correct verbatim retentions |

**All 114 triples read with `--verbose`.** Findings:

| key | en | zh | en′ | verdict |
|---|---|---|---|---|
| `label.modeStop` | Stop | 停止 → **停转** | "Stop" → **"Stall"** | **RE-AUTHORED** |
| `seg-mode-stop` (title) | Stop Mode | 停止模式 → **停转模式** | "Stop Mode" → **"Stall mode"** | **RE-AUTHORED** |
| `engage-btn` | Engage | 启动 | "Start" | **LEFT, reason recorded** |
| `seg-char-wobble` | Wobble | 摇摆 | "Wow" | accepted — the tape term for the same phenomenon; no control on this page to confuse it with |
| `label.themeWobbleWarp` | Wobble & Warp | 摇摆与扭曲 | "Wow and Flutter" | accepted — same relationship the English pair already has |
| `label.playback` | Playback | 播放 | "Play" | accepted — its only near neighbour is its own control's tip title 播放速率 |
| `seg-mode-scratch` | Scratch Mode | 刮擦模式 | "Scrub mode" | accepted — settled root; no scrub control on this page |
| `label.subtitle` | Varispeed Transport · A Field Guide | 变速走带 · 实地指南 | "Varispeed · Field Guide" | accepted — 走带 IS transport; the reader compressed a compound |
| `label.themeTapeStops` | Tape Stops | 磁带停转 | "Tape Stop" | accepted — number only |
| **`aria.envLength`** | Pass Length division | 通过长度分割 | **"By Length Division"** | **DEFERRED to D4**, see below |

### The re-author, and why only one side was qualified (M13's shape again)

The reader returned **"Start" for 启动** in **all seven bodies that quote it**, and **停止** (the
Stop **MODE** caption) sits on the same page. 启动/停止 is the strongest verb pair in the language:
a reader meets a large 启动 button beside a 停止 segment and reads a start/stop transport pair, when
停止 is a mode name and not a button.

Nothing mechanical could see this. Z5 is silent — each rendering IS the accepted root for its own
English. The downstream check looks for two keys mapping to **one** string, and these map to two
different strings. R3's screen fires only when both sides are glossary keys sharing a **root**, and
these are two **different** roots. Only the reverse read saw it.

**Only ONE side is qualified**, per M13, and the asymmetry is deliberate and recorded at the entry:

- `label.modeStop` 停止 → **停转**, with a `termNote`. 停转 names what the reel actually does, and
  matches this plugin's own **磁带停转** preset group and its product concept. Its tip title moved
  to 停转模式 in the same edit, so the caption and its own tooltip title stay in agreement.
- **启动 is LEFT.** It is not ambiguous about what it does — it starts the gesture — and the reader
  recovered that meaning correctly every one of the seven times.

**Round 2** — fresh salt (`round2.tsv`), fresh session, **different model** (`--model opus`) — on the
two re-authored rows plus the four they sit beside on the page. M12 clean, 6/6 ids in order.
It returned **"Stall"** for 停转 and **"Stall mode"** for 停转模式: **the pair is broken.** It
corrected nothing further, **so there is no round 3.**

### The one deferral

`aria.envLength` 通过长度分割 read back as **"By Length Division"** — the reader parsed 通过's first
character as the preposition *by/through* rather than the noun *pass*. **通过长度 is the settled
glossary root for `pass length`**, nothing on this page collides with it, and diverging from a
settled root in one plugin unilaterally is exactly what the glossary divergence report (D4) exists
to prevent. **Filed to D4, not fixed here.** Task 3 should carry it forward.

---

## Rows re-authored, and why

| # | key | from | to | reason |
|---|---|---|---|---|
| 1 | `label.modeStop` | 停止 | **停转** | M13 page collision with 启动 (Engage); qualified because it is the side ambiguous about what it does. `termNote` at the entry |
| 2 | `seg-mode-stop` title | 停止模式 | **停转模式** | keeps the caption and its own tooltip title in agreement after (1) |
| 3 | `aria.helpToggle` | 切换悬停帮助 | **开关悬停帮助** | Z5 fired: the settled glossary root for `Toggle hover help`. No reason to depart |

---

## Checked non-defects — read in full, deliberately left, recorded

1. **The `'settings'` body.** Reads *"Choose the language of this plugin, and turn the hover help on
   or off. Both choices are remembered with the session."* — it **names both** of the panel's
   controls and is **already true**. Left unchanged, and a probe in the verify block asserts it was
   not edited. Q8's whole-wave finding confirmed on the tracer: five waves of mechanical deletion
   have made deletion the reflex, and here the reflex would have stripped a true sentence.
2. **Zero form-4 nodes AND zero `inherit` declarations — MEASURED, not assumed.** The computed
   `ff` census over the visible en arm returns exactly two stacks: **168 nodes on `var(--serif)`**
   and **1 node on `Times`** (the `html` element, on the UA face). Zero-and-zero is a page with no
   instance of the class, not a page nobody measured — wave 4c's C4 converse, recorded as such.
3. **The `html` element on the UA `Times`.** Read, left, recorded: it renders no text and everything
   under `body` re-declares. It is the carrier no census entry accounts for.
4. **Zero naked generics.** All 13 `font-family` declarations go through one `var(--serif)` token,
   which names three families.
5. **Q12 — `scala-tuning-engine` non-consumption, verified two ways.** The CMake grep returns
   exactly six consumers — **O-Bassoon, O-Bowed, O-Contrabass, O-MicrotonalSampler, O-Reed,
   O-Wind** — none of this wave's five. A `find` for `tuning-panel.js` under each of the five returns
   **0**; no plugin here embeds a private copy either.
6. **P13 re-confirmed.** O-Tapestop declares seven `AudioParameterChoice`s and **none names a
   language** — measured by scanning each declaration's own text, not the containing file.

---

## Findings written down for Tasks 2 and 3 to inherit

1. **The gate-port shape.** O-ReverseDelay's repair transfers line for line. The three
   *enumerating* sites generalize; the three *reset-scoped* sites are left and classified at the
   line. `grep -c LANGUAGES`, never `grep -c vm`, decides whether the loader work is needed.
2. **The two-arm CMake reader is mandatory.** Cross-checked live against the other four:

   | plugin | one-arm | two-arm |
   |---|---|---|
   | O-Lyrica | 2.4.4 | 2.4.4 |
   | O-simpleBeatmaker | **EMPTY** | 1.2.1 |
   | O-simpleFM | **EMPTY** | 1.4.1 |
   | O-simplePhysicalModelSynth | **EMPTY** | 1.2.3 |

   Q9 holds exactly. O-Tapestop's own bump went on the `VERSION` line (an unquoted literal);
   the other three go on their `set()` line.
3. **The codec grep discipline.** Comment-stripped `zh-Hans` count = **2**; both `languageCode` and
   `languageIndex` proved **THREE-WAY by name**, never by a sweep's exit code. `find … -exec perl
   -CSD -ne '/\p{Script=Han}/'` over `Source/**/*.{h,cpp}` prints **nothing**, and the positive
   control on `js/i18n.js` **fired on the same run**. Widening the ternary is the entire C++ change.
4. **The blind-dispatch shape.** Emit twice for two salts, then build the chunks by resolving real
   ids through each manifest and taking the complement — **do not slice by line number**, because
   the emitter sorts by blinded id and the two emits are in different orders. Then M12 before ingest,
   both refusal controls, `--verbose`, every triple.
5. **Line-box ratios reconfirmed at four sizes**, each derived from the CONTENT box with padding and
   border subtracted first: **9px → 1.1111111, 9.5px → 1.0526316, 10px → 1.1, 11px → 1.0909091**.
   All four agree with M8's table. Put the pin **inside the rule that already owns the selector**,
   at that rule's own specificity, never in a new higher-specificity block (R8).
6. **`.env-plate`-style fixed-height containers absorb a wrap-count change.** A `wrap-count` finding
   is not automatically a defect; check `check-ui-labels` before pinning anything.

---

## Deviations

1. **The plan's `wrap-count` criterion of 0 was not met, and should not have been.** It reads **2**,
   and both findings are evidenced non-movers (one a pre-existing French wrap, one a Chinese shrink
   inside a fixed-height plate). No pin was added. Recorded rather than forced.
2. **One extra edit not in the plan's step list:** `aria.helpToggle` 切换悬停帮助 → 开关悬停帮助,
   forced by a Z5 finding on the first per-plugin lint run. A settled-root correction, not a
   judgement call.
3. **No `auval -v`**, by instruction. The triple was read off `auval -a` instead
   (`aufx OTsp OuDv`), which required no rescan because the install's registrar kill had already
   refreshed the registry.

---

## Corrections to the plan's own measurements

Instruction 18. **Four predictions were FALSE as stated.**

1. **`existing min-w / min-h / line-h / letter-sp = 0 / 0 / 0 / 0` for O-Tapestop is WRONG.**
   Measured on `styles.css` as found: **5 `line-height`**, **1 `min-width`**, **21
   `letter-spacing`**, plus several `white-space: nowrap`. The plan's derived claim that "every pin
   you add is the first on its selector and the R8 specificity grep comes back clean **by
   construction**" is therefore also false — R8 is **live** on this plugin. The grep was run per
   selector instead and the conclusion survived on the evidence: none of the nine selectors pinned
   here already declared `line-height`, and the four that DID (`.preset-nav` 1, `.env-hint` 1.5,
   `.help-toggle` 1, `.tooltip-body` 1.35) were left untouched. **A measured clean grep, not a
   constructed one** — which is precisely the distinction the plan itself was drawing.
2. **The verify block's `stats.get('en')` criterion of exactly `2` is WRONG — the correct number
   after the shipped port is `3`.** The generalized print block the plan *instructs* porting from
   O-ReverseDelay introduces `const e = stats.get('en');` as the **baseline** every other language is
   diffed against. So the shipped file carries 1 baseline read + 2 reset-scoped reads. The criterion
   as written would fail the very port it mandates. The load-bearing intent is intact and was checked
   directly: the two reset-scoped reads survive at raw L726 and L728, and `__setLanguage('en')` at
   L723.
3. **The verify block's "Fired at planning time: 4" for the two-language-literal alternation is
   WRONG — it fires 3.** The alternation only matches `'fr'`-side reads, and the file holds exactly
   three matching **lines**: L455 (the literal), L640 (`tipTextByLang.get('fr')`) and L661
   (`stats.get('fr')` — which shares its line with `stats.get('en')` and so counts once under
   `grep -c`). Shipped value is **0**, as required.
4. **`measure-ui --report all`'s `undeclared-font` screen is not the form-4 census the plan implies.**
   It reported **77 findings** on this page immediately after the table landed, on a plugin the plan
   correctly measured at **zero** form-4 nodes. The 77 are nodes whose stack names **no CJK face** —
   the *tail* half, not the *form-4* half. The genuine form-4 census (nodes taking their face from
   the UA stylesheet) had to be computed by grouping the JSON's `x.ff` over `lang === 'en' && vis`
   rows, exactly as Q11 describes, and it returns **1 node — the `html` element on `Times`, which
   renders no text**. Tasks 2 and 3 should not read `undeclared-font: N` as "N form-4 nodes".

**Predictions that held exactly:** the 114-row count, the 6 states, the 860 × 580 space-form frame,
the `--serif` token's contents and its 13 declarations, the zero form-4 / zero `inherit` /
zero naked-generic result, the raw line numbers L201 / L455 / L459 / L639 / L640 / L654 / L661 /
L671 / L673 / L674, the ~84-line comment-stripped offset, the enumeration probe firing **2** on the
tree as found, `VERSION 1.6.3` as an unquoted literal, the one-arm reader returning EMPTY on three
of the other four, the installed-family table (Garamond 0, EB Garamond 0, Times New Roman 4,
PingFang SC 6, Microsoft YaHei 0), the byte-identical codec at L159–160, the `'settings'` body being
already true, `svg-font-attr` 0, every O-ReverseDelay port line, and the clamp gate passing on the
tree as found.

---

## Commits

| hash | message | files |
|---|---|---|
| `1ad239b4` | `feat(O-Tapestop): Simplified Chinese at reviewed:'mt', clamp gate derives its language list` | `js/i18n.js`, `index.html`, `css/styles.css`, `Source/PluginProcessor.h`, `tests/ui_tooltip_clamp_check.js` |
| `04a7561a` | `feat(O-Tapestop): promote zh-Hans to reviewed:'bt', v1.7.0` | `js/i18n.js`, `CMakeLists.txt`, `CHANGELOG.md` |

Both path-scoped to `plugins/O-Tapestop`. No `git add -A`, no tag, nothing pushed. The only other
change in the tree is `.gsd/dispatch-isolation-sentinel.json`, which predates this work and was
never staged.
