---
title: "Chinese (zh-Hans) Localization Across the Ouaricon Plugin Suite"
created: 2026-09-01
last_verified: 2026-09-01
summary: "Measured reference for adding Simplified Chinese to the 43-plugin WebView i18n system: why zh-Hans is the right schema key and the right document.documentElement.lang value, why there is no silent-webfont-fallback hazard in this repo, the exact width law (zh width = character count x font-size) and its inversion into a per-term character budget, the eight repo-wide gate prerequisites that must land before the first zh-Hans entry exists, and a review model built on an automated back-translation gate because the developer cannot read Chinese."
domain: ui
type: research
keywords:
  - i18n
  - localization
  - chinese
  - zh-Hans
  - cjk
  - webview
  - typography
  - fonts
stages: [3]
---

# Chinese (zh-Hans) Localization Across the Ouaricon Plugin Suite

**Domain:** WebView i18n, CJK typography and font fallback, JUCE APVTS string persistence, repo-wide gate design

**Confidence:** HIGH on the repo facts (all measured, file:line cited) · MEDIUM on the CJK rendering advice (measured in headless Chromium on macOS plus cited sources) · MEDIUM on the translation-production recommendation (a judgement call, not a fact)

The staged, executable half of this work lives in
`.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md`.
This document is the "why"; that one is the "do". Neither repeats the other.

---

## Recommendation summary

| Question | Recommendation |
|---|---|
| **Variant** | **Simplified first.** Larger market; the Traditional pass is a downstream conversion of an existing table, not a fresh translation. |
| **Schema key** | **`zh-Hans`** — not `zh`, not `zh-CN`. One string does two jobs: it is the table key AND the value the canon writes to `document.documentElement.lang` (`scripts/i18n-canon.js:166`), which is what drives Han-unification font selection. `zh-Hant` slots in later with zero renaming. |
| **Fonts** | **No bundled webfont exists anywhere in `plugins/`** — every stack is system names (Garamond / Times New Roman / serif). There is nothing to silently fall through. Append an explicit CJK tail (`'PingFang SC', 'Microsoft YaHei', sans-serif`) to the *stacks that localized nodes resolve through* (~10 declarations per plugin, not 450), because Chromium's default `serif` for zh-Hans on Windows is SimSun, which is bitmap-blurry below 12px. |
| **Geometry** | **Chinese is the EASY direction.** Measured: zh width = `charCount x font-size` **exactly**; at parity font size, zh is wider on **5/32** representative captions (mixed-case, max +4.2px) and **1/32** on uppercase pages (+0.5px). French's width-pinned-abbreviation mechanism does **not** transfer — the zh analogue is a **character budget**: `maxChars = floor(cellWidthPx / fontSizePx)`. |
| **Review model** | **Machine draft + an automated back-translation gate**, with `reviewed` becoming a per-language enum `'mt' \| 'bt' \| 'native'`. The developer can review a *back-translation* in English; he cannot review Chinese. `'bt'` is the ship bar; `'native'` stays open. |
| **Pilot** | **O-Chorus** (29 entries) — it was the French pilot, it already carries measured px cliffs in its own header (62px wrap cliff, 50px gate cliff), and its zh pass is therefore a controlled comparison. Hard-case wave 2: **O-Octagon** (all-caps pinned speaker labels + its own `ui_layout_check`), **O-Bitrot** (inline-module controller + own clamp gate), **O-MicrotonalSampler** (largest table). |
| **Prerequisite gate changes** | **Eight, all before the first `zh-Hans` entry lands** — see §5. The largest is a **43-file same-commit canon sweep** (`i18n-canon.js:216`). |

---

## 1. Current state

| Fact | Value | Evidence |
|---|---|---|
| Plugins with `js/i18n.js` | **43** (33 under `Source/ui/public/js/`, 10 under `Resources/ui/js/`) | `find plugins -name i18n.js` [VERIFIED] |
| `export const LANGUAGES = ['en', 'fr'];` | **43 byte-identical copies** | `grep -rh 'LANGUAGES *=' ...` -> `43 export const LANGUAGES = ['en', 'fr'];` [VERIFIED] |
| Total table entries | **3,789** = 1,422 `I18N` (tooltip) + 2,367 `LABELS` | parsed every i18n.js in a VM [VERIFIED] |
| Unique EN short strings (titles + labels) | **1,917** across 3,789 occurrences -> **49% dedupe** | [VERIFIED] |
| EN short strings appearing in >1 place | **543**, covering **2,415 of 3,789 occurrences (64%)** | [VERIFIED] |
| Unique EN tooltip **bodies** | **1,284** across 1,331 occurrences (only 25 repeat) — **~33,350 words of prose** | [VERIFIED] |
| Top shared strings | `Settings` 75 · `Language` 68 · `Interface language` 44 · `Save` 33 · `Mix` 30 · `Off` 26 · `Output` 25 · `On` 24 · `Release` 24 · `Previous preset` 24 | [VERIFIED] |
| C++ codec uniformity | **43 identical** `static juce::String languageCode (int i) { return i == 1 ? "fr" : "en"; }`; **42 identical** `languageIndex`; the outlier is **O-MultiBandCompressor** (`PluginProcessor.h:95` — one space of whitespace difference only, semantics identical) | [VERIFIED: plugins/*/Source/PluginProcessor.h] |
| Canon copies carrying the fr clamp | **47** files contain `applyI18n(code === 'fr' ? 'fr' : 'en')` — **43 shipping** + 4 `.planning/i18n-index-draft.html` drafts | [VERIFIED] |
| `<option value="fr">Fran&ccedil;ais</option>` | **43** index.html files | [VERIFIED] |
| Per-plugin test files referencing `'fr'` | **25** (`ui_tip_render_check.js` x22, `ui_tooltip_clamp_check.js` x3) | [VERIFIED: plugins/*/tests/*.js] |
| `<html lang="...">` in markup | **45/45 `lang="en"`** — the canon overwrites it at runtime | [VERIFIED] |
| `<meta charset="utf-8">` | **45/45 present** | [VERIFIED] |
| Bundled webfont files (`.woff*`/`.ttf`/`.otf`) under `plugins/` | **ZERO** | `find plugins -type f \( -name '*.woff*' -o -name '*.ttf' -o -name '*.otf' \)` returned nothing [VERIFIED] |
| `@font-face` in shipped UI CSS | **ZERO** (the one hit is `O-Orbit/libs/SAF/docs/doxygen/CustomDoxygen.css`, a vendored doc theme) | [VERIFIED] |

The single most consequential number in this table is **543 shared strings covering 64% of all
occurrences**. That set is the glossary, it is computable today, and settling it before any
translator is dispatched is the one decision that prevents the French rollout's 267 divergent
renderings from recurring.

---

## 2. Variant, schema key

**Ship Simplified (`zh-Hans`) first.** The Traditional pass, if ever wanted, is a script
conversion of a finished table plus a terminology diff — a fraction of the cost of the first
pass. Shipping both at once triples the review surface with no reviewer.

**Use `zh-Hans` as the table key and the stored code, not `zh` and not `zh-CN`.**

- `zh-CN`/`zh-TW` are a *region* stand-in for what is really a *script* distinction. W3C:
  "People had to bend something like `zh-CN` ... to mean Simplified Chinese, even in Singapore."
  The script subtags "should improve consistency and accuracy, and [are] already becoming widely
  used." [CITED: w3.org/International/articles/bcp47/]
- Bare `zh` is under-specified for the one job the value actually does at runtime. `applyI18n`
  writes the key straight into `document.documentElement.lang` (`scripts/i18n-canon.js:164-166`),
  and Blink's Han fallback walks "a prioritized list of locales: the language of a node as defined
  in HTML ..." and uses `scriptForHan()` of the first locale that `hasScriptForHan()`.
  [CITED: chromium.googlesource.com/.../LocaleInFonts.md] WebKit does the equivalent via
  `-webkit-locale`. Declaring `zh-Hans` removes the ambiguity that produces Japanese or
  Traditional glyph shapes for unified codepoints.
- **The key and the `lang` value must be the same string** — that is already how the canon works,
  and it is the reason not to invent a separate mapping.

**Hyphen safety.** `zh-Hans` contains a hyphen. The known repo trap — `juce_add_binary_data`
STRIPS hyphens from *filenames* — does **not** apply: no new file is added, the third language
rides in the existing `i18n.js`. Object literal keys need quoting (`'zh-Hans': { t: ... }`); the
canon reads `entry[lang]`, `LANGUAGES.includes(lang)` and `sel.value`, all of which handle a
hyphenated string unchanged. [VERIFIED: scripts/i18n-canon.js:114-224]

---

## 3. Fonts, rendering, and typography

### 3.1 There is no silent-fallback webfont hazard

The premise that a bundled webfont with no CJK glyphs would swallow Chinese is **false for this
repo**: there are zero font files and zero `@font-face` rules in any shipped UI. Every stack is a
system-name list. [VERIFIED]

The declared stacks (top of the distribution, 450 declarations repo-wide, ~10 distinct per plugin):

| Count | Declaration |
|---|---|
| 179 | `font-family: inherit` |
| 140 | `'Garamond', 'Times New Roman', serif` |
| 88 | `'Garamond', serif` |
| 87 | `var(--serif)` |
| 49 | `Garamond, 'Times New Roman', serif` |
| 43 | `'Garamond', 'Georgia', serif` |
| 36 | `monospace` |
| 32 | `'Courier New', monospace` |

None of these families carries Han glyphs, so CSS per-character fallback drops through to the
generic (`serif`/`monospace`) and the browser's per-script default for the resolved locale takes
over.

- **macOS (WKWebView):** generic serif for zh-Hans resolves to **Songti SC**; the system UI face is
  **PingFang SC** (El Capitan+). A serif Chinese face beside a Garamond page is aesthetically
  coherent — the default outcome here is *good*. [CITED: chinesemac.org/pages/osx11.html,
  support.apple.com/103203]
- **Windows 10/11 (WebView2/Chromium):** generic serif for zh-Hans is typically **SimSun**, which
  "previously, 12px was the limit for clear display ... font sizes smaller than 12px would become
  blurry due to the lack of embedded bitmaps." **Microsoft YaHei** displays "exceptionally clear"
  at 12-14px and ships on Windows 7+. [CITED: baike.baidu.com Microsoft YaHei]

**Recommendation:** append a CJK tail to the stacks that localized nodes actually resolve through
— `..., 'PingFang SC', 'Microsoft YaHei', sans-serif`. This is language-independent (Latin still
resolves to Garamond first, so **English geometry is byte-unchanged** and no `:lang()` selector is
needed), and it is ~10 declarations per plugin, not 450, if scoped by measuring which stacks
`[data-i18n]` nodes inherit. A `:root:lang(zh-Hans)` rule is the **wrong** instrument: any element
that declares its own `font-family` outranks it by specificity.

### 3.2 Minimum legible size — the real content risk

Repo-wide `font-size` declarations: **10 at 6px, 52 at 7px, 246 at 8px, 519 at 9px, 599 at 10px,
442 at 11px** — 827 declarations at <=8px and 1,946 at <=10px. [VERIFIED]

Chinese below 12px is at or past the legibility floor (SimSun's bitmap cliff is exactly 12px;
research on Traditional-character readability puts 14px as the comfortable target). The suite's
entire caption tier sits below that.

**Do not apply a blanket zh font-size bump.** Measured cost of a `[lang^="zh"]` 12px floor against
the current English sizes:

| Page style | en@9px | en@10px | en@11px |
|---|---|---|---|
| mixed-case | zh@12px wider on **16/32** | **10/32** | **7/32** |
| `text-transform: uppercase` | **5/32** | **2/32** | **2/32** |

**Recommendation:** ship zh at **parity font size** (no CSS size change -> `check-ui-labels`
assertion 7 stays an honest EN<->ZH delta, and the measured geometry is safe: 5/32 mixed-case,
1/32 uppercase). Where a pinned cell has slack, bump *that node* to 11-12px under `[lang^="zh"]`
and let `check-ui-labels` prove it. Record the <=9px tier as a **known, disclosed limitation**,
not a silent one.

### 3.3 Wrapping, punctuation, hyphens, quotes

| Question | Answer |
|---|---|
| `word-break` / `overflow-wrap` / `line-break` | **No change needed.** Measured: a 9-character no-space Han string in a 40px box wrapped to 4 lines under default `line-break: auto`. CSS breaks between Han characters natively and already avoids starting a line with the full-width period or comma. The repo declares only 5 such properties in total. [VERIFIED] |
| `hyphens: auto` | **Does not exist in any plugin CSS.** The 19 grep hits are the canon's own comment line. No zh branch needed. [VERIFIED] |
| `quotes` CSS | **Does not exist.** zh-Hans convention is the same curly quotes as English. No change. [VERIFIED] |
| Full-width punctuation | **Required in zh prose**, and the French U+00A0 rules have **no analogue** — the full-width forms carry their own half-em sidebearing. A zh lint must **forbid** U+00A0 before punctuation, the exact inverse of `i18n-fr-lint.js` T3/T4/T5. **Reusing the French lint on zh would be actively wrong.** [VERIFIED: scripts/i18n-fr-lint.js:38-52] |
| `text-transform: uppercase` | **A no-op on Han.** All-caps pages (O-Marimba, O-Polystutter, O-Octagon speaker labels) render Han unchanged beside uppercase Latin acronyms. Lint **C1 has no zh analogue** and must be skipped, not ported. |

### 3.4 Line-box height — the one real geometry trap

Measured EN vs ZH line-box height, `'Garamond','Times New Roman',serif`:

| `line-height` | 9px | 10px | 11px |
|---|---|---|---|
| `1` | en 9.00 / zh 9.00 | 10.00 / 10.00 | 11.00 / 11.00 |
| `1.2` | 10.80 / 10.80 | 12.00 / 12.00 | 13.19 / 13.19 |
| `1.35` | 12.14 / 12.14 | 13.50 / 13.50 | 14.84 / 14.84 |
| **`normal`** | **10.00 / 13.00** | **11.00 / 14.00** | **12.00 / 16.00** |

**Unitless `line-height` is font-independent — zero geometry shift.** The repo is overwhelmingly
unitless (79x `1`, 54x `1.2`, 28x `1.4`, 24x `1.35`, 14x `1.1`). But `line-height: normal` makes a
Han line box **+30% taller**, which is a `check-ui-labels` assertion 7 geometry failure vector.
Explicit offenders: **4x `normal`, 2x `initial`** — *plus* every node that declares no line-height
and has no ancestor that does, which inherits `normal` from the UA. That inherited set is the
larger, unmeasured exposure.

**Recommendation:** do not pre-emptively add a global `line-height` (that would move English
geometry — a regression). Let the zh arm of `check-ui-labels` assertion 7 name each offender, then
pin *that node's* computed line-height explicitly so both languages agree.

> **Risk A5, Stage 2 (2026-09-03) — STILL CHROMIUM-MEASURED, UNCONFIRMED IN WKWebView.** The
> O-Chorus pilot reconfirmed the 9px row on the real shipping page: `.knob-label` measured
> **10.00 px in English and 13.00 px in Chinese**, exactly the +30% above, and the recommendation
> held — three unitless pins at the measured EN ratio (10.00/9 = 1.1111) closed 25 of the 26
> assertion-7 movers with English unmoved. **But the re-measurement in the shipped WKWebView was
> NOT taken, because it cannot be with what the plugin exposes:** O-Chorus's WebView bridge has
> twelve native functions (ten preset, `getUiLanguage`, `setUiLanguage`) and no
> `evaluateJavascript` path, so nothing can read `getComputedStyle` from inside the AU/VST3 host.
> Taking it would mean shipping a style-reading debug hook in a release plugin. The figure
> therefore remains a headless-Chromium number and should still not be quoted as an exact
> WKWebView value. It is recorded here as a known limitation rather than dropped.
>
> Stage 2 also found a geometry mover that is **not** a line-height at all and that §3.4 does not
> predict: a `<select>` whose intrinsic width Chromium derives from the **selected** option's font
> run, so the control measured 65 px with a Latin endonym selected and 64 px with the Han one —
> even though the option set is language-invariant. Any plugin whose language selector sits in a
> `space-between` row will show it. Fix is a width pin at the existing English intrinsic.

---

## 4. Width geometry

**Measured law:** in every resolved CJK face tested, a Han glyph advance is **exactly 1.0 em**. A
two-character Han string measured 18.0px at 9px, 20.0px at 10px, 22.0px at 11px — no rounding, no
kerning. So:

> **zh caption width (px) = character count x font-size (px), exactly.**

That is the single most useful fact for the planner: **every caption's Chinese width is computable
analytically without rendering anything**, and the width budget inverts into a character budget:

> **`maxChars = floor(availableWidthPx / fontSizePx)`**

Measured EN vs ZH at parity size, 32 representative shared captions
(`'Garamond','Times New Roman',serif`):

| Page style | 9px | 10px | 11px |
|---|---|---|---|
| mixed-case | zh wider on **5/32** | **5/32** | **5/32** |
| uppercase | **1/32** | **1/32** | **1/32** |

The five mixed-case offenders, measured at parity 10px (en px -> zh px): `Mix` 16.7->20,
`Rate` 18.3->20, `Save` 19.5->20, `Size` 17.2->20, `Next preset` 45.8->50. **Maximum overrun at
parity is ~+4.2px**, and it is confined to **3-4-character English words rendered as 2 Han
characters**. On uppercase pages only `Mix` exceeds, by +0.5px.

For contrast, Chinese is *dramatically* narrower on long labels: `Interface language` 74.1px ->
4 Han characters = 40px at 10px; `Modulation depth` 71.4 -> 4 characters = 40px.

**Conclusions:**

1. `check-ui-labels` assertions **4, 5, 6 and 7 should pass far more easily for zh than they did
   for fr.** The pressure inverts from overflow to *under-fill* and *per-glyph legibility*.
2. The French **width-pinned abbreviation** mechanism (`Profondeur -> Prof.`, `Etalement ->
   Etal.`) has **no Chinese equivalent** — Chinese has no abbreviations. Its analogue is choosing a
   **2-character rendering instead of a 3- or 4-character one**, and the glossary must carry that
   as a **character-count budget per term**, not an abbreviation list.
3. The residual risk set is small and enumerable in advance: any caption whose English is <=5
   characters in a cell pinned to its English width. There are **271 unique EN label strings of <=5
   characters** out of 1,313 (185 occurrences at 1-3 chars, 446 at 4-5). [VERIFIED]

---

## 5. Schema and runtime changes

### 5.1 Repo-wide prerequisites — these MUST land BEFORE the first plugin gets a `zh-Hans` entry

Otherwise `check-i18n` exits non-zero on the pilot and on all 43.

| # | File:line | Change | Why blocking |
|---|---|---|---|
| **P1** | `scripts/i18n-canon.js:216` | `.then((code) => applyI18n(code === 'fr' ? 'fr' : 'en'))` -> **`.then((code) => applyI18n(code))`** | The clamp is already redundant: `applyI18n` does `LANGUAGES.includes(lang) ? lang : 'en'` (`i18n-canon.js:162`). Making it language-agnostic means **a fourth language never needs another 43-file sweep.** Fix it once, correctly. |
| **P2** | **43 shipping copies** of the canon body | Re-sync byte-identically **in the same commit as P1** | Assertion 6 byte-compares the region against `i18n-canon.js` (`check-i18n.js:784`). A canon change with any copy unsynced fails that plugin. Targets are the 43 files listed by `grep -rl "applyI18n(code === 'fr'" plugins` **minus** the 4 `.planning/i18n-index-draft.html` drafts. |
| **P3** | `scripts/check-i18n.js:497-498` | `LANGUAGES.join(',') === 'en,fr'` / `[1] LANGUAGES is exactly ['en','fr'] — got ...` -> accept `en,fr` **or** `en,fr,zh-Hans` | Hard-coded two-language assertion. Accepting **both** shapes is what makes an incremental 43-plugin rollout possible instead of a big-bang. |
| **P4** | `scripts/check-i18n.js:516, 548, 605` | `for (const lang of ['en','fr'])` -> derive from the file's own `LANGUAGES` | Same reason; a 2-language plugin and a 3-language plugin must both pass mid-rollout. |
| **P5** | `scripts/check-i18n.js:576-578` (tooltips) **and `scripts/check-i18n.js:623` (LABELS)** | `typeof (...fr \|\| {}).reviewed !== 'boolean'` -> boolean for `fr`, one of `'mt'\|'bt'\|'native'` for `zh-Hans` — **at both sites** | **There are TWO reviewed-flag assertions, not one.** `:576-578` guards `I18N` (tooltips); `:623` guards `LABELS_EARLY`. Patching only the tooltip site leaves the labels table still demanding a boolean, and every zh label entry fails assertion [5]. Do **not** touch the fr semantics at either site. See §6.2. |
| **P6** | `scripts/check-ui-labels.js:598, 658, 669, 703, 717, 851` | `['en','fr']` -> the plugin's `LANGUAGES`; assertions 5/6/7 become per-non-English-language deltas against EN | **Six** hard-coded loops, not seven. Line 635 sits inside the EN->EN control-spread block (`const enControl = []` and its comment), not a language loop — it must not be edited. Verify with `grep -n "\['en', *'fr'\]" scripts/check-ui-labels.js`. Without P6 the zh geometry is never measured — a silent vacuous pass. |
| **P7** | `scripts/i18n-extract.js:1252` | `body.push("        fr: { t: 'TODO', b: '', reviewed: false },")` -> emit a `'zh-Hans'` row too | The skeleton generator is where new keys enter; missing this produces zh-less keys forever. |
| **P8** | **New:** `scripts/i18n-zh-glossary.js`, `scripts/i18n-zh-lint.js`, `scripts/i18n-zh-backtranslate.js` | See §6 | The French lesson: **267 divergent renderings appeared before a glossary existed.** Do not repeat it. |

Also touched, non-blocking: **25 per-plugin test files** under `plugins/*/tests/` reference `'fr'`;
they need a zh arm, or at minimum must not regress.

### 5.2 Per-plugin (x43)

| Site | Edit |
|---|---|
| `js/i18n.js` | `LANGUAGES = ['en', 'fr', 'zh-Hans']` and a third entry on **every** key in `I18N` (1,422 total) and `LABELS` (2,367 total) |
| `index.html` | `<option value="zh-Hans">&#31616;&#20307;&#20013;&#25991;</option>` beside the fr option — **as numeric entities**, matching the existing `Fran&ccedil;ais` convention. See the encoding note in §7. |
| `PluginProcessor.h` | `languageCode (int i) { return i == 1 ? "fr" : i == 2 ? "zh-Hans" : "en"; }` and `languageIndex (...) { return s == "fr" ? 1 : s == "zh-Hans" ? 2 : 0; }` — both remain pure ASCII, so the `juce::String(const char*)` ASCII-only trap is avoided by construction |
| canon body in `app.js` / `index.html` | the P1/P2 sweep (already counted above) |

### 5.3 APVTS migration — NOT needed. Confirmed.

`kUiLanguageProp` is stored as a **string** on the APVTS state tree:

```cpp
// plugins/O-simpleFM/Source/PluginProcessor.cpp:388-390
parameters.state.setProperty (kUiLanguageProp,
                              languageCode (uiLanguage.load (std::memory_order_acquire)),
                              nullptr);
```

and read back with an `isVoid()` gate at `PluginProcessor.cpp:415-417`. Adding a third code changes
nothing about the round-trip:

- A pre-zh session ("en"/"fr") loads unchanged in a zh-capable build.
- A zh session loaded by an **older** build hits `languageIndex()`'s "anything not `fr` -> 0" and
  degrades to **English** — a graceful, already-designed fallback, not a corruption.
  [VERIFIED: plugins/O-simpleFM/Source/PluginProcessor.h:153-154, PluginProcessor.cpp:412-417]
- The value is never an `AudioParameterChoice`, so no automation lane or preset diff is affected.

The one caveat worth naming: `kUiLanguageProp` is a named constant in only **4** plugins; the other
39 write the literal `"uiLanguage"` inline, via several plumbings (APVTS property vs XML
attribute). The *property name* is uniform; the *code around it* is not, so this is 43 hand-edits,
not a sed.

---

## 6. Translation production and review model

### 6.1 The corpus, sized

| Bucket | Count | Notes |
|---|---|---|
| Total entries needing a `zh-Hans` value | **3,789** | 1,422 tooltip + 2,367 label |
| Unique short strings (labels + tip titles) | **1,917** | 49% of the corpus is duplication |
| Shared short strings (>1 site) | **543**, covering **64% of all occurrences** | **This is the glossary. Settle it first.** |
| Unique tooltip **bodies** (prose) | **1,284**, ~**33,350 words** | Almost no reuse (25 repeat). This is the bulk of the work. |

The French rollout produced 267 divergent renderings *before* a glossary existed. The measured
shared-string set here (543 terms / 64% coverage) is the direct analogue and is available *now*,
before a single translator is dispatched.

### 6.2 Review model

The developer reads French; the `reviewed: true` flip on 2026-08-31 meant literally "the developer
read it." **That lane is closed for Chinese.** The options and the call:

| Option | Verdict |
|---|---|
| Native-speaker review | Best quality, but planning around a reviewer who may not exist blocks the task indefinitely. **Not the primary lane.** |
| Raw machine draft, shipped | Repeats the exact failure the French glossary was built to fix, with no reviewer able to see it. **No.** |
| **Machine draft + automated back-translation gate** | **Recommended.** `zh -> en'` by a *second, independent* pass, diffed against the source `en`. A meaning drift shows up **in English**, which the developer can read. It is repeatable, it is a CI gate, and it is exactly the shape of gate this repo already builds (`i18n-fr-lint.js` exits 2). |

**Schema:** make `reviewed` a **per-language enum for zh only**, leaving `fr`'s boolean untouched:

```js
'zh-Hans': { t: '...', b: '...', reviewed: 'bt' }   // 'mt' | 'bt' | 'native'
```

- `'mt'` — machine draft, unchecked. Fails the ship gate.
- `'bt'` — back-translation reviewed by the developer against the English source. **This is the
  ship bar.**
- `'native'` — a qualified Chinese reader signed off. Aspirational; leaves the door open without
  blocking.

Both reviewed-flag assertions in `check-i18n.js` — the tooltip site at `:576-578` and the LABELS
site at `check-i18n.js:623` — become: boolean for `fr`, one of the three strings for `zh-Hans`.
Shipping at `'bt'` is a **disclosed** quality level, not a silent one — which is the whole design
intent of the existing flag.

New tool: **`scripts/i18n-zh-backtranslate.js`** — emits `en -> zh -> en'` triples for diffing,
per plugin, `--verbose` for all. Report first, gate once the corpus is at zero findings (the exact
lifecycle `i18n-fr-lint.js` went through).

### 6.3 The zh lint ruleset

`i18n-fr-lint.js` T1-T7 are **French typography and must not be reused.** The zh ruleset:

| Code | Rule | Rationale |
|---|---|---|
| **Z1** | Full-width punctuation in zh prose; ASCII `,.:;?!()` forbidden outside a Latin/unit token | The zh convention; ASCII punctuation in Han prose is the #1 MT tell |
| **Z2** | **No U+00A0** before `: ; ! ? %` — the inverse of fr T3/T4/T5 | Full-width forms carry their own sidebearing |
| **Z3** | **No Traditional-only characters** in a `zh-Hans` table | Mixed-variant output is the second most common MT artifact. Derive the character set from OpenCC data rather than a hand list |
| **Z4** | **Latin/CJK spacing consistency** — one plain U+0020 between a Latin/digit run and a Han run, everywhere or nowhere; pick a space | Follow the fr precedent's reasoning about U+202F: a thin space (U+2009/U+200A) has no glyph in some faces and would render as a box where no gate looks |
| **Z5** | **Glossary conformance** (the G1 analogue) against `scripts/i18n-zh-glossary.js` | Same mechanism, same 543-term problem |
| **Z6** | **Character budget** — a caption exceeding `maxChars = floor(cellWidthPx / fontSizePx)` characters | The abbreviation analogue; see §4 |
| **Z7** | **No full-width Latin/digits** | Classic MT artifact |
| **—** | **C1 casing: SKIP for zh** | Han has no case; `text-transform: uppercase` is a no-op |
| **F1** | Forbidden-word list | Keep the mechanism, new content |

Preserved unchanged: `sameAsEn: true` (needed for `LFO`, `MIDI`, `dB`, `Hz`), `termNote`
exemptions, `I18N_EXEMPT`. Assertion [4] ("no straight passthrough without `sameAsEn`") applies to
zh identically.

---

## 7. Repo-specific pitfalls

### P-1 · The 43-copy canon sweep is the single largest atomic change

`scripts/i18n-canon.js:216` hard-codes `'fr'`. Changing it invalidates all 43 byte-compared copies
at once (`check-i18n.js:784`, assertion 6). **This must be one commit**, path-scoped per CLAUDE.md,
and it must include `scripts/i18n-canon.js` itself. Getting P1's *form* right — `applyI18n(code)`,
relying on the guard already at `i18n-canon.js:162` — means this sweep happens **once, ever**, not
once per language.

### P-2 · Encoding: safe, but for a non-obvious reason, and with thin margin

- **`i18n.js` is safe unconditionally.** 36 of 43 plugins serve it with **no `charset`**
  (`juce::String("application/javascript")` — e.g. `plugins/O-Chorus/Source/PluginEditor.cpp:297`;
  O-Bells uses `"text/javascript"` at `PluginEditor.cpp:1061`). This does not matter: **module
  scripts are always decoded as UTF-8 regardless of Content-Type charset**, and every one of the 45
  index.html files loads its controller with `type="module"`. [CITED: WebKit commit c40873d,
  "Module scripts should always decode using UTF-8"] French accents already prove the path
  end-to-end — `i18n.js` files are UTF-8 with **8,525 lines carrying literal non-ASCII**.
  [VERIFIED: `file`, `LC_ALL=C grep`]
- **`index.html` is the thinner margin.** Many plugins serve it as bare `text/html` with no charset
  (O-Bells, O-Wind, O-Lyrica, O-Formant, O-Gain, O-Reed, O-Contrabass, O-Polystutter, ... ~20),
  leaving the `<meta charset>` prescan (1024-byte window) as the only signal. Measured offsets
  cluster at **846-859 bytes** — inside the window, with ~165 bytes of headroom. Five plugins sit
  **past 1024** (O-Octagon 2920, O-simpleGrain 1994, O-simpleSubtractive 1836, O-simpleAdditive
  1833, O-Tapestop 1393) but all five *do* send `text/html; charset=utf-8`, so HTTP wins and they
  are fine today. [VERIFIED]
  **The Chinese `<option>` endonym would be the first literal non-ASCII the HTML path ever
  carries** — French dodged it entirely by writing `Fran&ccedil;ais` as an entity.
  **Recommendation: write the endonym as numeric entities (`&#31616;&#20307;&#20013;&#25991;`),
  matching the existing convention, and separately add `charset=utf-8` to every index.html
  Content-Type as a belt-and-braces cleanup.**

### P-3 · Keep every Chinese character out of C++

`juce::String(const char*)` is ASCII-only. Nothing forces a violation: `languageCode` returns
`"zh-Hans"` (ASCII), parameter display names stay English by locked decision, factory preset names
stay ASCII because **the preset name IS the filename**
(`OuariconPresetManager.h:283-285`). **Rule: all Chinese lives in `i18n.js` and `index.html`. Zero
Chinese literals in `Source/`.**

### P-4 · Two UI roots, and the 4 draft files

33 plugins under `Source/ui/public/js/`, 10 under `Resources/ui/js/` (O-Bassoon, O-Bells, O-Bowed,
O-FreqPulse, O-Lyrica, O-MicrotonalSampler, O-Orbit, O-Reed, O-SpectralShaper, O-Wind). Also:
`grep` for the canon returns **47** files — 4 are `plugins/*/.planning/i18n-index-draft.html`
(O-AnalogSaturation, O-Bitrot, O-Emulator, O-SimpleReverb). They are not served and not gated;
decide explicitly whether to sync or delete them, rather than discovering the discrepancy
mid-sweep.

### P-5 · `check-ui-labels` must be extended, not merely re-run

Assertion 2 is the vacuity guard (`MIN_LANG_DIFF_FRACTION` of labels must differ). Assertions 5/6/7
are **deltas against English**, phrased in French terms throughout the source. Ported naively by
adding `'zh-Hans'` to the six `['en','fr']` loops, the zh pass will *work* — but the failure
messages will still say "French," which is exactly the "mysterious regression in a file that never
mentions French" the file's own header warns about (`check-ui-labels.js:28-30`). Relabel while
extending.

### P-6 · O-MultiBandCompressor is the C++ outlier

`plugins/O-MultiBandCompressor/Source/PluginProcessor.h:95` differs from the other 42 by whitespace
only (`static int         languageIndex` vs `static int          languageIndex`). Semantics
identical — but any sed-based sweep keyed on the exact 42-copy string will silently skip it.

---

## 8. Rollout shape

Six stages, each sized for one `/gsd-quick` execution. The full staged plan — per-stage files,
gates, commit scope, size and copy-paste invocation — lives in
`.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md`
and is not duplicated here.

- **Stage 0 — repo-wide prerequisites.** P1-P8 above, one or two commits, ~45 files. Exit: all
  gates pass on 43/43 *with the tables still at two languages*.
- **Stage 1 — glossary and lint before any translator.** 543 shared strings with a character budget
  per term; `i18n-zh-lint.js` and `i18n-zh-backtranslate.js` as reports, not gates.
- **Stage 2 — pilot: O-Chorus.** Same pilot as French, so the pass is a controlled comparison; its
  header already records the px cliffs the character budget converts directly (62px wrap cliff /
  10px = 6 chars; 50px gate cliff / 10px = 5 chars).
- **Stage 3 — hard cases before any volume wave.** O-Octagon, O-Bitrot, O-MicrotonalSampler — every
  structural variant at once, cheaper to fix before 39 plugins carry the pattern.
- **Stage 4 — volume waves.** The corpus is skewed: the top 8 plugins hold 1,530 of 3,789 entries
  (40%); the bottom 15 hold under 40 each.
- **Stage 5 — repo-wide QA and the definition of done.** `i18n-zh-lint` to 0 repo-wide,
  back-translation reviewed on 43/43, `boot-all-uis` 43/43 clean, `check-ui-labels` zh arm 0
  geometry moved, `auval` PASS x43 — and an explicit ship bar of `'bt'`.

---

## Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|---|---|---|
| A1 | The candidate Chinese renderings used in the width measurements are plausible; the *arithmetic* they demonstrate (width = chars x px, exactly) is measured and holds for any Han string, but the specific term choices are the glossary's job, not this document's | §4 | None to the geometry law; the per-term table is illustrative only |
| A2 | Simplified is the larger commercial market for this suite | §2 | A Traditional-first decision would reverse the variant order; the schema key `zh-Hans`/`zh-Hant` recommendation is unaffected either way |
| A3 | No native Chinese reviewer is available | §6.2 | If one is, the review model upgrades to `'native'` with no schema change — the enum was designed for exactly that |
| A4 | Chromium's default generic-serif for zh-Hans on Windows is SimSun | §3.1 | If it is YaHei on Win11, the CJK-tail edit is a polish item rather than a legibility fix. Measured on macOS only |
| A5 | WKWebView's Han fallback behaves like the Chromium measurement taken here (all width/height numbers came from headless Chromium on macOS) | §3.4, §4 | The em-advance law is a property of the CJK faces themselves and will hold; the `line-height: normal` +30% figure should be re-measured on WKWebView before it is quoted as an exact number. **OPEN after Stage 2 (2026-09-03):** reconfirmed in Chromium on the real page (10.00 -> 13.00 px at 9px) and the pin strategy works, but the WKWebView measurement was NOT taken — the shipped bridge exposes no `evaluateJavascript` and no style-reading native function, so it would require shipping a debug hook. See the note in §3.4. |

## Sources

**Primary (measured in-repo):** `scripts/i18n-canon.js:114-224` (notably `:162` guard, `:166`
`documentElement.lang`, `i18n-canon.js:216` fr clamp), `scripts/check-i18n.js:28-80, 293, 480-640,
766-805` (notably `:497-498` assertion [1], `:516, 548, 605` language loops, `:576-578` and
`check-i18n.js:623` reviewed-flag assertions, `:784` canon byte-compare),
`scripts/check-ui-labels.js:20-80, 465-880` (six `['en','fr']` loops at 598, 658, 669, 703, 717,
851), `scripts/i18n-fr-lint.js:20-90`, `scripts/i18n-extract.js:1252`, `scripts/serve-ui.js:524-541`,
`plugins/O-simpleFM/Source/PluginProcessor.{h:150-197,cpp:382-420}`,
`plugins/O-simpleFM/Source/PluginEditor.cpp:40-115, 238-268`,
`plugins/O-Chorus/Source/PluginEditor.cpp:294-299`,
`plugins/O-Bells/Source/PluginEditor.cpp:1058-1063`,
`plugins/O-MultiBandCompressor/Source/PluginProcessor.h:93-95`, all 43 `js/i18n.js`, all 45
`index.html`. Geometry and encoding measurements run via Playwright 1.62.1 (headless Chromium) and
`grep -abo`.

**Secondary (cited):**
- [Locale uses in Fonts — Chromium/Blink](https://chromium.googlesource.com/chromium/src/+/HEAD/third_party/blink/renderer/platform/fonts/LocaleInFonts.md)
- [Understanding the New Language Tags — W3C](https://www.w3.org/International/articles/bcp47/) · [RFC 5646](https://datatracker.ietf.org/doc/html/rfc5646)
- [Module scripts should always decode using UTF-8 — WebKit](https://github.com/WebKit/WebKit/commit/c40873d23eeadf0fe542b79eca7652f9f19689f5)
- [Chinese in Mac OS X El Capitan (PingFang / Songti)](https://chinesemac.org/pages/osx11.html) · [Fonts included with macOS](https://support.apple.com/en-sg/103203)
- [Microsoft YaHei](https://baike.baidu.com/en/item/Microsoft%20YaHei/1462878) · [Best Practices for Chinese Layout — Bobby Tung](https://bobtung.medium.com/best-practice-in-chinese-layout-f933aff1728f)

**Prior art (in-repo):** the French rollout, `.planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/`
— CONTEXT.md, FR-GLOSSARY.md, RESEARCH.md, and the Stage K/L/M/N/O briefs.

---

**Valid until:** ~2026-10-01. Repo facts drift with any i18n commit; re-run the counts before
planning if `scripts/i18n-*.js` changes.
