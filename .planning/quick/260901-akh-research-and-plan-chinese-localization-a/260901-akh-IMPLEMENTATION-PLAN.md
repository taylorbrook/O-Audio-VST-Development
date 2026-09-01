# Chinese (zh-Hans) Localization — Staged Implementation Plan

**Created:** 2026-09-01
**Scope:** all 43 plugins carrying `js/i18n.js`, plus the six repo-level `scripts/i18n-*.js` and
`scripts/check-*.js` gates.
**Reference:** every "why" in this plan is answered by
[`research/i18n-zh-hans-localization.md`](../../../research/i18n-zh-hans-localization.md). This
document repeats no measurement it does not need in order to act. If a number here and a number
there disagree, the research document is authoritative.

**Shape:** six stages. Each is sized for one `/gsd-quick` execution with a `/clear` between, and
each carries its own files, gates, commit scope, size and a copy-paste invocation. Stages run in
order — Stage 0 is a hard prerequisite for every later stage, and Stage 1 is a hard prerequisite
for Stage 2 onward.

---

## Decisions to confirm before Stage 0

These six are the developer's, not an executor's. Nothing below Stage 0 should start until (a),
(b), (c) and (d) are settled; (e) and (f) can settle during Stage 0.

| # | Decision | Recommendation | Consequence of the other branch |
|---|---|---|---|
| **(a)** | **Simplified first, or Traditional first?** | **Simplified (`zh-Hans`) first.** Larger market; the Traditional pass is a script conversion of a finished table plus a terminology diff, not a fresh translation. | Traditional-first reverses the stage order but changes nothing structural — the schema-key recommendation in (b) is identical either way, and `zh-Hant` slots into the same `LANGUAGES` array. Shipping *both* at once triples the review surface with no reviewer, which is the branch to avoid. |
| **(b)** | **Schema key: `zh-Hans`, `zh`, or `zh-CN`?** | **`zh-Hans`.** One string does two jobs: the table key AND the value the canon writes to `document.documentElement.lang` (`scripts/i18n-canon.js:166`), which is what drives Han-unification font selection. | `zh-CN` is a *region* stand-in for a *script* distinction and would need renaming the day `zh-Hant` arrives. Bare `zh` is under-specified for the font-selection job and can surface Japanese or Traditional glyph shapes for unified codepoints. See research §2. |
| **(c)** | **Review model: `reviewed` as a per-language enum `'mt' \| 'bt' \| 'native'`?** | **Yes, for `zh-Hans` only.** `fr` keeps its boolean, untouched, at both assertion sites. | A plain boolean cannot distinguish "machine draft nobody checked" from "back-translation the developer read." Collapsing them makes the ship gate meaningless, which is the exact failure the flag exists to prevent. |
| **(d)** | **Ship at `'bt'`, or hold the release until a native reader signs off?** | **Ship at `'bt'`.** A back-translation the developer read against the English source is a *disclosed* quality level; `'native'` stays open as a later upgrade with no schema change. | **This is the single decision that determines whether the work has an end.** Holding for `'native'` makes the entire rollout blocked on a reviewer who may never be available — the French item 27 situation, except French closed only because the developer can read French. That lane is closed for Chinese. |
| **(e)** | **Pilot plugin: O-Chorus?** | **Yes.** It was the French pilot, so the zh pass is a controlled comparison, and its `i18n.js` header already records measured px cliffs (62px wrap, 50px gate) that convert directly into character budgets. | Any other pilot forfeits the comparison and starts the px-cliff measurement from scratch. |
| **(f)** | **The four unserved `plugins/*/.planning/i18n-index-draft.html` drafts — sync or delete?** | **Delete.** They are not served, not gated, and their only effect is to make `grep -rl "applyI18n(code === 'fr'" plugins` return 47 instead of 43. | Syncing them means maintaining four copies of the canon forever with no gate protecting them, which is how they drifted in the first place. Whichever branch is chosen, it must be chosen *before* the P1/P2 sweep so the sweep's target list is unambiguous. |

Affected plugins for (f): O-AnalogSaturation, O-Bitrot, O-Emulator, O-SimpleReverb.

---

## Stage 0 — Repo-wide prerequisites

**Goal:** make every i18n gate language-agnostic, so that a 2-language plugin and a 3-language
plugin both pass while the rollout is mid-flight. No plugin gains a `zh-Hans` entry in this stage.

The prerequisite ids **P1–P8** are the same ids used in `research/i18n-zh-hans-localization.md` §5.1
— the two documents cross-reference by id.

| # | File:line | Exact edit | Verification | Why it must land BEFORE the first zh-Hans entry |
|---|---|---|---|---|
| **P1** | `scripts/i18n-canon.js:216` | `.then((code) => applyI18n(code === 'fr' ? 'fr' : 'en'))` -> `.then((code) => applyI18n(code))` | `grep -n "applyI18n(code)" scripts/i18n-canon.js` returns line 216; `grep -c "code === 'fr'" scripts/i18n-canon.js` returns 0 | The clamp silently discards any code that is not `fr`, so a stored `zh-Hans` would load as English with no error anywhere. The guard at `i18n-canon.js:162` (`LANGUAGES.includes(lang) ? lang : 'en'`) already does this job correctly, so `applyI18n(code)` is not just a fix — it means **a fourth language never needs another 43-file sweep.** |
| **P2** | the **43 shipping copies** of the canon body | Re-sync byte-identically, **in the same commit as P1** | `node scripts/check-i18n.js` assertion 6 (canon byte-compare, `check-i18n.js:784`) PASS on 43/43 | Assertion 6 byte-compares each plugin's embedded canon region against `scripts/i18n-canon.js`. Change P1 without the sweep and all 43 plugins fail at once. Target list: `grep -rl "applyI18n(code === 'fr'" plugins` **minus** the four `.planning/i18n-index-draft.html` drafts (decision (f)). |
| **P3** | `scripts/check-i18n.js:497-498` | `LANGUAGES.join(',') === 'en,fr'` -> accept `'en,fr'` **or** `'en,fr,zh-Hans'`; update the message string on the same line | run `check-i18n` against an unmodified plugin (must PASS) and against a scratch 3-language copy (must also PASS) | This is assertion [1], the hard-coded two-language check at `check-i18n.js:497`. Accepting **both** shapes is precisely what makes an incremental per-plugin rollout possible instead of a 43-plugin big-bang. |
| **P4** | `scripts/check-i18n.js:516, 548, 605` | `for (const lang of ['en','fr'])` -> derive from the file's own `LANGUAGES` | `grep -c "\['en', *'fr'\]" scripts/check-i18n.js` returns 0 | Same reason as P3. A literal `['en','fr']` skips every zh entry, which is worse than failing: the gate goes green on unchecked content. |
| **P5** | `scripts/check-i18n.js:576-578` (tooltips) **and `scripts/check-i18n.js:623` (LABELS)** | `typeof (...fr \|\| {}).reviewed !== 'boolean'` -> boolean for `fr`, one of `'mt'\|'bt'\|'native'` for `zh-Hans` — **at both sites**; also update the assertion text at `:43`, `:578` and `:625` | `grep -n "reviewed" scripts/check-i18n.js` shows the enum branch at both `:576-578` and `:623`; a scratch entry with `reviewed: 'mt'` passes, one with `reviewed: 'oops'` fails | **There are TWO reviewed-flag assertions, not one.** `:576-578` guards `I18N` (tooltips); `:623` guards `LABELS_EARLY`. Patch only the tooltip site and every one of the 2,367 zh *label* entries fails assertion [5] — a failure that appears only once translation starts, i.e. at the worst possible time. Do **not** touch the `fr` semantics at either site. |
| **P6** | `scripts/check-ui-labels.js:598, 658, 669, 703, 717, 851` | `['en','fr']` -> the plugin's `LANGUAGES`; assertions 5/6/7 become per-non-English-language deltas against EN; relabel the "French" wording in the failure messages | `grep -n "\['en', *'fr'\]" scripts/check-ui-labels.js` returns nothing; a dry run on one plugin shows the loop reading `LANGUAGES` | **SIX loops, not seven.** Line **635** sits inside the EN->EN control-spread block (`const enControl = []` and its preceding comment) and is **not** a language loop — editing it corrupts the vacuity control. Without P6 the zh geometry is never measured and assertion 7 passes vacuously. |
| **P7** | `scripts/i18n-extract.js:1252` | the `fr: { t: 'TODO', b: '', reviewed: false },` emit gains a sibling `'zh-Hans': { t: 'TODO', b: '', reviewed: 'mt' },` row | run the extractor on a scratch plugin; the generated skeleton carries all three languages | The skeleton generator is where new keys enter the system. Miss it and every key added after Stage 0 is born zh-less, forever, silently. |
| **P8** | **new:** `scripts/i18n-zh-glossary.js`, `scripts/i18n-zh-lint.js`, `scripts/i18n-zh-backtranslate.js` | created in **Stage 1**, not here — P8 is listed at Stage 0 only to keep the id set complete and to record the ordering law | n/a at Stage 0 | The French rollout produced **267 divergent renderings before a glossary existed**. No per-plugin dispatch may begin until P8 exists and is settled. |

**Ordering inside the stage:** P1+P2 must be one commit (see below). P3–P7 may share a second
commit. P8 is deferred to Stage 1.

**Files touched:** `scripts/i18n-canon.js`, `scripts/check-i18n.js`, `scripts/check-ui-labels.js`,
`scripts/i18n-extract.js`, and the 43 shipping canon copies under
`plugins/*/Source/ui/public/js/app.js`, `plugins/*/Resources/ui/js/app.js` and the `index.html`
files that carry an inline canon region. Approximately 47 files across two commits. Zero
`i18n.js` tables change in this stage.

**Gates:** `node scripts/check-i18n.js` ALL PASS on 43/43 **with the tables still at two
languages** (this is the exit criterion — the gates became language-agnostic without any zh content
existing yet); assertion 6 canon byte-compare PASS on 43/43; a `check-ui-labels` dry run on at
least one plugin proving the loops now read `LANGUAGES` rather than a literal; `boot-all-uis` 43/43
clean, 0 DEAD bindings.

**Commit scope:** two path-scoped commits, per CLAUDE.md. The canon sweep must be atomic —
assertion 6 fails on any plugin whose copy is unsynced.
```bash
# commit 1 — P1 + P2, the atomic 43-file canon sweep
git commit -- scripts/i18n-canon.js plugins

# commit 2 — P3..P7, the gate and extractor changes
git commit -- scripts/check-i18n.js scripts/check-ui-labels.js scripts/i18n-extract.js
```
Re-check `git branch --show-current` and `git status --short` immediately before each commit;
another session shares this checkout's index.

**Size:** ~47 files, 2 commits, no version bumps, no builds. Half a session. The mechanical part is
large but the reasoning is small — the risk concentrates entirely in getting P1's *form* right so
the sweep never has to happen again.

**Invocation:**
```
/gsd-quick Stage 0 of the zh-Hans rollout: make the i18n gates language-agnostic. Follow
.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md
"Stage 0 — Repo-wide prerequisites", items P1 through P7. Read research/i18n-zh-hans-localization.md
sections 5.1 and 7 first. Two path-scoped commits: P1+P2 atomic (canon + 43 copies), then P3-P7.
Exit criterion: check-i18n ALL PASS on 43/43 with every table still at two languages, and a
check-ui-labels dry run showing the loops read LANGUAGES not a literal. Add NO zh-Hans entries.
```

---

## Stage 1 — zh-Hans glossary and lint

**Goal:** settle the terminology and build the three zh tools **before any translator is
dispatched**. No plugin is touched in this stage.

1. **`scripts/i18n-zh-glossary.js`** over the **543 shared English strings** that cover **64% of
   all 3,789 occurrences**. Each term carries: the agreed `zh-Hans` rendering, a `charCount`, and a
   **character budget** derived from the tightest cell that term appears in
   (`maxChars = floor(cellWidthPx / fontSizePx)`, research §4). The budget replaces French's
   width-pinned abbreviation list — Chinese has no abbreviations, so the lever is choosing a
   2-character rendering over a 3- or 4-character one.
2. **`scripts/i18n-zh-lint.js`** implementing **Z1–Z7** plus a G1-equivalent glossary-conformance
   check and the `reviewed` enum check:
   - Z1 full-width punctuation in zh prose; ASCII `,.:;?!()` forbidden outside a Latin/unit token
   - Z2 **no U+00A0** before `: ; ! ? %` — the deliberate inverse of French T3/T4/T5
   - Z3 no Traditional-only characters in a `zh-Hans` table (derive the set from OpenCC data, not a
     hand list)
   - Z4 Latin/CJK spacing consistency — one plain U+0020, everywhere or nowhere; never a thin space
   - Z5 glossary conformance against `i18n-zh-glossary.js`
   - Z6 character budget: a caption exceeding `floor(cellWidthPx / fontSizePx)` characters
   - Z7 no full-width Latin or digits
   - preserved unchanged: `sameAsEn: true` (`LFO`, `MIDI`, `dB`, `Hz`), `termNote`, `I18N_EXEMPT`
   - **explicitly NOT ported:** French **T1–T7** (French typography) and **C1** (casing — Han has
     no case, `text-transform: uppercase` is a no-op on it). Reusing them would be actively wrong,
     not merely useless.
3. **`scripts/i18n-zh-backtranslate.js`** emitting `en -> zh -> en'` triples for diffing, per
   plugin, `--verbose` for all. The second pass must be *independent* of the pass that produced the
   Chinese, or the triple proves nothing.

**Lifecycle — state this in each tool's header:** all three ship as **reports first**, exit code 0
regardless of findings, and are promoted to **gates** (exit 2) only once the pilot is at zero
findings. That is the exact lifecycle `scripts/i18n-fr-lint.js` went through, and it is what keeps
a half-built lint from blocking Stage 2.

**Ordering law, stated plainly:** the French rollout produced 267 divergent renderings *before* a
glossary existed. The shared-string set is computable **today**. **No per-plugin dispatch begins
until the glossary is settled.**

**Files touched:** `scripts/i18n-zh-glossary.js` (new), `scripts/i18n-zh-lint.js` (new),
`scripts/i18n-zh-backtranslate.js` (new). Zero files under `plugins/`.

**Gates:** each tool runs clean on the current 43 plugins (which contain no zh yet, so the correct
output is "0 zh entries found, nothing to check" — a vacuity result, not a pass); the glossary
parses; a scratch fixture with a known Z1/Z2/Z3/Z7 violation is detected by each rule. If a rule
cannot be shown to fire on a deliberate violation, it is not implemented, it is decorative.

**Commit scope:** the three new files are untracked, so `git add` must precede the commit —
`git commit -- <path>` sees only tracked files.
```bash
git add scripts/i18n-zh-glossary.js scripts/i18n-zh-lint.js scripts/i18n-zh-backtranslate.js
git commit -- scripts/i18n-zh-glossary.js scripts/i18n-zh-lint.js scripts/i18n-zh-backtranslate.js
```

**Size:** 3 new scripts, 1 commit, 543 glossary terms to settle. The glossary is the long pole, not
the code. One full session, possibly two if the terminology is debated. If any tool needs an npm
dependency, that install carries a package-legitimacy check — this is the first stage in the
rollout that installs anything.

**Invocation:**
```
/gsd-quick Stage 1 of the zh-Hans rollout: build the glossary and the zh lint tooling, touch no
plugin. Follow .planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md
"Stage 1 — zh-Hans glossary and lint". Read research/i18n-zh-hans-localization.md section 6 first.
Create scripts/i18n-zh-glossary.js (543 shared strings, each with a character budget),
scripts/i18n-zh-lint.js (Z1-Z7 + glossary conformance + reviewed enum; do NOT port French T1-T7 or
C1) and scripts/i18n-zh-backtranslate.js. All three are REPORTS, exit 0, not gates. Exit criterion:
every rule demonstrably fires on a deliberate violation fixture. git add before git commit --.
```

---

## Stage 2 — Pilot: O-Chorus

**Goal:** carry one plugin (29 entries) end to end and prove the pattern before 42 more copy it.
O-Chorus was the French pilot, so this is a controlled comparison.

1. `js/i18n.js`: `LANGUAGES = ['en', 'fr', 'zh-Hans']`, and a `'zh-Hans'` value on **every** key in
   both `I18N` and `LABELS`. Keys are quoted (`'zh-Hans': { ... }`) — the hyphen is legal in a
   quoted object key and the canon reads `entry[lang]` unchanged.
2. `index.html`: `<option value="zh-Hans">&#31616;&#20307;&#20013;&#25991;</option>` beside the fr
   option, **written as numeric entities**, matching the existing `Fran&ccedil;ais` convention.
   This is the first literal-non-ASCII risk the HTML path has ever faced; entities sidestep it
   entirely (research §7, P-2).
3. **CJK font tail**: append `, 'PingFang SC', 'Microsoft YaHei', sans-serif` to the ~10 stacks the
   localized nodes actually resolve through — *not* all 450 declarations, and *not* via a
   `:root:lang(zh-Hans)` rule (any element declaring its own `font-family` outranks it). Measure
   which stacks `[data-i18n]` nodes inherit before editing. Latin still resolves to Garamond first,
   so **English geometry must be byte-unchanged**.
4. `PluginProcessor.h`: `languageCode (int i) { return i == 1 ? "fr" : i == 2 ? "zh-Hans" : "en"; }`
   and the matching `languageIndex`. Both stay **pure ASCII** — zero Chinese characters anywhere
   under `Source/`.
5. **`line-height` audit** as an explicit step: run the zh arm of `check-ui-labels` assertion 7 and
   let it *name* the offending nodes, then pin each named node's computed line-height so both
   languages agree. `line-height: normal` makes a Han line box +30% taller; the inherited-`normal`
   set is larger than the 4 explicit declarations and only the gate can enumerate it. Do **not**
   add a global line-height — that moves English geometry, which is a regression.
6. **Character budgets from O-Chorus's own recorded cliffs**, via
   `maxChars = floor(cellWidthPx / fontSizePx)`:
   - 62px wrap cliff / 10px = **6 characters**
   - 50px gate cliff / 10px = **5 characters**
   These two numbers are the pilot's Z6 budget and belong in the glossary entries for the terms
   that land in those cells.
7. Version bump + `CHANGELOG.md` entry; `./scripts/build-and-install.sh O-Chorus`.

**Files touched:** `plugins/O-Chorus/Source/ui/public/js/i18n.js`,
`plugins/O-Chorus/Source/ui/public/index.html`, the CSS carrying the ~10 target stacks,
`plugins/O-Chorus/Source/PluginProcessor.h`, `plugins/O-Chorus/CHANGELOG.md`, the plugin's
`CMakeLists.txt` version, and `PLUGINS.md`.

**Gates:** `check-i18n` PASS (all assertions, now including the enum branch at
`check-i18n.js:623`); `check-ui-labels` zh arm with **0 geometry moved** on both the EN and the ZH
pass; `i18n-zh-lint` **0 findings**; back-translation triples read and reconciled against the
English source; `boot-all-uis` clean, 0 DEAD bindings; `auval -a | grep -i chorus` PASS after
install.

**Commit scope:** one commit for the plugin, one for the registry row.
```bash
git commit -- plugins/O-Chorus PLUGINS.md
```

**Size:** 1 plugin, 29 entries, ~6 files, 1 version bump, 1 build. Half a session. The value is not
the 29 entries — it is that every structural question (font tail scope, entity encoding,
line-height offenders, budget arithmetic) gets answered once, cheaply.

**Invocation:**
```
/gsd-quick Stage 2 of the zh-Hans rollout: pilot O-Chorus. Follow
.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md
"Stage 2 — Pilot: O-Chorus", all seven items in order. Read research/i18n-zh-hans-localization.md
sections 3, 4 and 7 first. Stages 0 and 1 must already be committed. Zh-Hans entries on every I18N
and LABELS key; endonym as numeric entities; CJK font tail on ~10 stacks only; zero Chinese
literals under Source/. Exit criterion: 0 geometry moved, 0 i18n-zh-lint findings, back-translation
read, auval PASS. Commit path-scoped: git commit -- plugins/O-Chorus PLUGINS.md
```

---

## Stage 3 — Hard-case wave

**Goal:** exercise every structural variant at once, on three plugins (~518 entries, approx),
before 39 plugins carry the pattern. Anything these surface is far cheaper to fix here.

| Plugin | Entries (approx.) | Why it is hard | What it teaches |
|---|---|---|---|
| **O-Octagon** | 131 | All-caps **pinned speaker labels**; carries its own `ui_layout_check`; its `index.html` `<meta charset>` sits at **byte 2920**, past the 1024-byte prescan window | Whether uppercase-page geometry survives (research measured 1/32 uppercase captions wider at parity — this is the plugin that tests it against *pinned* cells), whether a per-plugin layout gate needs its own zh arm, and whether the `charset=utf-8` Content-Type is genuinely carrying the encoding for the five past-window plugins |
| **O-Bitrot** | 117 | **Inline-module controller** (not the standard `app.js` shape) plus its own tooltip clamp gate; also one of the four plugins carrying an unserved `.planning/i18n-index-draft.html` | Whether the P1/P2 canon sweep actually reached a non-standard controller layout, and whether the clamp gate's thresholds hold for Han text — which wraps between characters, not at spaces |
| **O-MicrotonalSampler** | 270 | **Largest table** in the suite; `Resources/ui/` root rather than `Source/ui/public/`; English **pluralization inlined in its JS** rather than living in the i18n table | The `Resources/ui/` path variant (10 plugins share it), and the pluralization problem — Chinese has no plural inflection, so inlined `n === 1 ? 'voice' : 'voices'` logic is untranslatable where it stands and must move into the table before it can carry a zh value |

**Files touched:** `plugins/O-Octagon/**`, `plugins/O-Bitrot/**`,
`plugins/O-MicrotonalSampler/**` (i18n.js, index.html, CSS font tails, PluginProcessor.h,
CHANGELOG.md, version), plus each plugin's own `tests/` arm where one exists, plus `PLUGINS.md`.

**Gates:** per plugin — `check-i18n` PASS, `check-ui-labels` zh arm 0 geometry moved,
`i18n-zh-lint` 0 findings, back-translation read, `auval` PASS. Plus the two plugin-specific gates:
O-Octagon's `ui_layout_check` and O-Bitrot's tooltip clamp check, both extended with a zh arm
rather than merely re-run.

**Commit scope:** one commit per plugin, so a failure in one does not strand the others.
```bash
git commit -- plugins/O-Octagon PLUGINS.md
git commit -- plugins/O-Bitrot PLUGINS.md
git commit -- plugins/O-MicrotonalSampler PLUGINS.md
```

**Size:** 3 plugins, ~518 entries, 3 commits, 3 version bumps, 3 builds. One full session, possibly
two — O-MicrotonalSampler's pluralization extraction is unbounded until it is looked at.

**Invocation:**
```
/gsd-quick Stage 3 of the zh-Hans rollout: the hard-case wave — O-Octagon, O-Bitrot,
O-MicrotonalSampler. Follow .planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md
"Stage 3 — Hard-case wave". Stage 2 (O-Chorus) must be committed and its lessons folded into the
glossary. Same seven-item per-plugin procedure as Stage 2, plus: extend O-Octagon's ui_layout_check
and O-Bitrot's clamp gate with a zh arm; move O-MicrotonalSampler's inlined English pluralization
into the i18n table before giving it a zh value. One commit per plugin. Report anything structural
you hit BEFORE the volume waves copy it.
```

---

## Stage 4 — Volume waves

**Goal:** the remaining **39 plugins**, in seven waves of five or six. Each wave mixes one heavy
plugin with several light ones so no wave is all-heavy, and each wave is one `/gsd-quick`.

Entry counts below are **approx.** — produced by a grep heuristic, within a few entries of the
VM-parsed figures in the research document (grep total 3,733 vs parsed 3,789). Use them for
sizing, not for reporting. Reproduce with:

```bash
for f in $(find plugins -name i18n.js | sort); do \
  p=$(echo "$f" | cut -d/ -f2); \
  n=$(grep -cE "^\s*'?[A-Za-z0-9_.-]+'?:\s*\{\s*(en|$)" "$f"); \
  echo "$n $p $f"; done | sort -rn
```

**UI-root split — this decides the file paths in every wave.** 33 plugins keep their UI under
`Source/ui/public/js/`; **10 use `Resources/ui/js/`**: O-Bassoon, O-Bells, O-Bowed, O-FreqPulse,
O-Lyrica, O-MicrotonalSampler, O-Orbit, O-Reed, O-SpectralShaper, O-Wind. Marked **[R]** below.

**Corpus skew:** the top 8 plugins hold **1,530 of 3,789 entries (40%)**; the bottom 15 hold under
40 each. That is why the waves are mixed rather than sequential.

### Wave 4a — ~392 entries
| Plugin | Entries (approx.) | Root |
|---|---|---|
| O-Prism | 262 | `Source/ui/public/` |
| O-Comp | 34 | `Source/ui/public/` |
| O-Freeze | 31 | `Source/ui/public/` |
| O-Texture | 26 | `Source/ui/public/` |
| O-Bass | 24 | `Source/ui/public/` |
| O-AnalogSaturation | 15 | `Source/ui/public/` |

### Wave 4b — ~340 entries
| Plugin | Entries (approx.) | Root |
|---|---|---|
| O-IntonationPad | 199 | `Source/ui/public/` |
| O-DigiDelay | 31 | `Source/ui/public/` |
| O-AnalogEQ | 30 | `Source/ui/public/` |
| O-Tremolo | 29 | `Source/ui/public/` |
| O-SimpleReverb | 29 | `Source/ui/public/` |
| O-Emulator | 22 | `Source/ui/public/` |

### Wave 4c — ~428 entries
| Plugin | Entries (approx.) | Root |
|---|---|---|
| O-Bells | 186 | **[R]** `Resources/ui/` |
| O-Gain | 63 | `Source/ui/public/` |
| O-SpectralShaper | 47 | **[R]** `Resources/ui/` |
| O-Detune | 47 | `Source/ui/public/` |
| O-Bassoon | 44 | **[R]** `Resources/ui/` |
| O-TextureForge | 41 | `Source/ui/public/` |

### Wave 4d — ~457 entries
| Plugin | Entries (approx.) | Root |
|---|---|---|
| O-Formant | 182 | `Source/ui/public/` |
| O-MultiBandCompressor | 70 | `Source/ui/public/` |
| O-ReverseDelay | 69 | `Source/ui/public/` |
| O-Marimba | 69 | `Source/ui/public/` |
| O-FreqPulse | 67 | **[R]** `Resources/ui/` |

O-MultiBandCompressor is the **C++ outlier**: `PluginProcessor.h:95` differs from the other 42 by
one space of whitespace. Any sed keyed on the exact 42-copy `languageIndex` string skips it
silently. Hand-edit and verify.

### Wave 4e — ~492 entries
| Plugin | Entries (approx.) | Root |
|---|---|---|
| O-Lyrica | 167 | **[R]** `Resources/ui/` |
| O-simpleBeatmaker | 85 | `Source/ui/public/` |
| O-simpleFM | 84 | `Source/ui/public/` |
| O-Tapestop | 79 | `Source/ui/public/` |
| O-simplePhysicalModelSynth | 77 | `Source/ui/public/` |

### Wave 4f — ~487 entries
| Plugin | Entries (approx.) | Root |
|---|---|---|
| O-Wind | 119 | **[R]** `Resources/ui/` |
| O-Contrabass | 103 | `Source/ui/public/` |
| O-Reed | 90 | **[R]** `Resources/ui/` |
| O-Bowed | 88 | **[R]** `Resources/ui/` |
| O-GrainScatter | 87 | `Source/ui/public/` |

O-Reed carries a known open defect (its tuning tab registers none of the 19 native fns it calls).
That is **out of scope** for this wave — do not fix it here, and do not let it block the i18n pass.

### Wave 4g — ~591 entries
| Plugin | Entries (approx.) | Root |
|---|---|---|
| O-simpleGrain | 115 | `Source/ui/public/` |
| O-simpleSampler | 109 | `Source/ui/public/` |
| O-simpleSubtractive | 97 | `Source/ui/public/` |
| O-Polystutter | 91 | `Source/ui/public/` |
| O-Orbit | 91 | **[R]** `Resources/ui/` |
| O-simpleAdditive | 88 | `Source/ui/public/` |

O-Orbit contains the `libs/SAF` **submodule** — never stage a path inside it.

**Files touched:** per plugin, the same six sites as Stage 2 — `js/i18n.js`, `index.html`, the ~10
CSS font-tail declarations, `PluginProcessor.h`, `CHANGELOG.md`, the version — plus `PLUGINS.md`
once per wave.

**Gates:** per plugin, every wave: `check-i18n` PASS; `check-ui-labels` zh arm **0 geometry
moved**; `i18n-zh-lint` **0 findings**; back-translation read; `boot-all-uis` clean, 0 DEAD;
`auval` PASS. A wave is not done until all of its plugins are green — no partial waves.

**Commit scope:** one commit per plugin, path-scoped, with the registry row folded in at the end of
the wave.
```bash
git commit -- plugins/<Name>            # once per plugin
git commit -- PLUGINS.md                # once at the end of the wave
```

**Size:** 39 plugins, ~3,187 entries (approx.), 7 waves, 39 version bumps, 39 builds. One session
per wave; seven sessions total. This is the bulk of the calendar time and almost none of the
difficulty — Stages 2 and 3 are where the thinking happens.

**Invocation:** (substitute the wave letter and its plugin list)
```
/gsd-quick Stage 4 wave <a|b|c|d|e|f|g> of the zh-Hans rollout: <plugin list>. Follow
.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md
"Stage 4 — Volume waves", the table for that wave. Stages 0-3 must be committed. Same per-plugin
procedure as Stage 2; note the [R] plugins use Resources/ui/js/ not Source/ui/public/js/. One
path-scoped commit per plugin, PLUGINS.md once at the end. Exit criterion: every plugin in the wave
green on check-i18n, check-ui-labels zh arm 0 geometry moved, i18n-zh-lint 0 findings, auval PASS.
```

---

## Stage 5 — Repo-wide QA and review pass

**Goal:** prove the whole suite at once, then answer the only question that decides whether this
work has an end.

1. `i18n-zh-lint` **0 findings repo-wide**, run with `--strict` across all 43 — and confirm the
   flag is actually wired (the French rollout left `--strict` unwired as an open item; do not
   inherit that).
2. Back-translation reviewed on **43/43**. The triples are read against the English source, not
   skimmed.
3. `boot-all-uis` **43/43 clean, 0 DEAD bindings**, in all three languages.
4. `check-ui-labels` zh arm **0 geometry moved** on 43/43.
5. `check-i18n` ALL PASS on 43/43, with `LANGUAGES` now `['en','fr','zh-Hans']` everywhere.
6. `auval` **PASS x43**.
7. Promote `i18n-zh-lint.js` and `i18n-zh-backtranslate.js` from **reports to gates** (exit 2), now
   that the corpus is at zero findings — the same promotion `i18n-fr-lint.js` received.

**Then define done.** The developer cannot read Chinese. The `reviewed` enum is what makes that
survivable rather than silent:

- **`'mt'`** — machine draft, unchecked. **Fails the ship gate.** No entry may ship at `'mt'`.
- **`'bt'`** — the developer read an **independent back-translation** of that entry against its
  English source and reconciled the meaning. **This is the ship bar.**
- **`'native'`** — a qualified Chinese reader signed off. **Aspirational. Stays open. Not a
  blocker.**

Shipping at `'bt'` is a **disclosed** quality level, not a hidden one — that is the entire design
intent of the flag, and it is the same intent behind `fr`'s boolean, which meant literally "the
developer read it." The Chinese analogue of "the developer read it" is "the developer read a
faithful English rendering of it." Anything stronger requires a reviewer who may never exist, and
planning around one has no termination condition.

**Files touched:** `scripts/i18n-zh-lint.js`, `scripts/i18n-zh-backtranslate.js` (report -> gate
promotion), `scripts/i18n-zh-glossary.js` (final reconciliation), plus any per-plugin corrections
the repo-wide run surfaces, plus `PLUGINS.md`.

**Gates:** all six checks in the numbered list above, on 43/43, with no exemptions and no
"expected failures." A single amber is a stage failure.

**Commit scope:**
```bash
git commit -- scripts/i18n-zh-lint.js scripts/i18n-zh-backtranslate.js scripts/i18n-zh-glossary.js
git commit -- plugins/<Name>    # per plugin, only where a correction was needed
git commit -- PLUGINS.md
```

**Size:** 1-3 commits plus whatever corrections the repo-wide run surfaces. One session if the
waves were clean; two if the back-translation review turns up systematic drift, which is exactly
what it exists to catch.

**Invocation:**
```
/gsd-quick Stage 5 of the zh-Hans rollout: repo-wide QA and the review pass. Follow
.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md
"Stage 5 — Repo-wide QA and review pass", all seven items. All 43 plugins must already carry
zh-Hans. Run i18n-zh-lint --strict to 0 repo-wide, back-translation reviewed 43/43, boot-all-uis
43/43 clean, check-ui-labels zh arm 0 geometry moved, check-i18n ALL PASS, auval PASS x43. Then
promote the lint and back-translate tools from reports to gates. Ship bar is 'bt': no entry ships
at 'mt'; 'native' stays open and is not a blocker.
```

---

## Risks

| Risk | Why it matters | Mitigation |
|---|---|---|
| **WKWebView line-height re-measurement (assumption A5)** | Every geometry number in the research document came from headless Chromium on macOS. The em-advance law (`width = chars x font-size`) is a property of the CJK faces and will hold anywhere, but the **`line-height: normal` +30%** figure is a rendering-engine measurement and should not be quoted as exact until it is re-taken in WKWebView. | Re-measure during Stage 2 on the actual O-Chorus build, not in a browser. If the figure differs, correct `research/i18n-zh-hans-localization.md` §3.4 before Stage 3 — the plan cites it, so a stale number propagates into 42 plugins. |
| **The sub-9px legibility tier** | 827 `font-size` declarations sit at <=8px and 1,946 at <=10px. Chinese below 12px is at or past the legibility floor (SimSun's bitmap cliff is exactly 12px). A blanket bump would move English geometry, which is a regression. | Ship at parity size, bump only individual nodes with proven slack under `[lang^="zh"]`, and **record the <=9px tier as a disclosed limitation** in the Stage 5 report. A known limitation is a decision; an unknown one is a bug. |
| **O-MultiBandCompressor whitespace outlier** | `PluginProcessor.h:95` differs from the other 42 by one space. Any sed-based sweep keyed on the exact 42-copy string skips it **silently** — the file simply does not match, no error is raised. | Hand-edit in Wave 4d and verify with an explicit grep, not with the sweep's own exit code. |
| **The four unserved `i18n-index-draft.html` files** | They make the canon grep return 47 instead of 43, so a sweep that trusts the grep either edits four dead files or produces an off-by-four that looks like a missed plugin. | Resolve decision (f) **before** Stage 0 runs. Deleting them is the recommendation. |
| **Shared-checkout index race** | Two sessions in one checkout share `.git/index` and HEAD. A concurrent session's staging can join a commit in the gap between checking and committing. | Path-scope every commit (`git commit -- <paths>`), never `git add -A`, and re-check `git branch --show-current` and `git status --short` immediately before each commit — not once at session start. |
| **Back-translation that is not independent** | If the same engine that produced the Chinese also produces the English back-translation, the triple round-trips its own vocabulary and reads clean while the Chinese is wrong. The gate goes green on unchecked content. | Require a *different* pass for the reverse direction, and record which one in the tool's output. A back-translation whose provenance is unrecorded proves nothing. |

## Effort by stage

| Stage | Plugins | Entries (approx.) | Commits | Builds | Sessions |
|---|---|---|---|---|---|
| 0 — prerequisites | 0 (43 canon copies) | 0 | 2 | 0 | 0.5 |
| 1 — glossary and lint | 0 | 0 (543 terms settled) | 1 | 0 | 1-2 |
| 2 — pilot O-Chorus | 1 | ~28 | 1 | 1 | 0.5 |
| 3 — hard cases | 3 | ~518 | 3 | 3 | 1-2 |
| 4 — volume waves (a-g) | 39 | ~3,187 | 39 + 7 | 39 | 7 |
| 5 — repo-wide QA | 43 (verification) | — | 1-3 | 0 | 1-2 |
| **Total** | **43** | **~3,733 grep / 3,789 parsed** | **~54** | **43** | **~11-14** |

Roughly three quarters of the calendar time is Stage 4, and roughly all of the risk is Stages 0-3.
