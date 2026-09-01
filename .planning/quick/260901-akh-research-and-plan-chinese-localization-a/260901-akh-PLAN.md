---
task: 260901-akh-research-and-plan-chinese-localization-a
type: execute
mode: quick
autonomous: true
staged: false

# DOCUMENTATION-ONLY. Two files created, nothing else touched.
files_modified:
  - research/i18n-zh-hans-localization.md
  - .planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md

# MUTABLE-SCOPE AUTHORITY: the two paths above are the ENTIRE edit scope.
# No plugin, no script, no gate, no CHANGELOG, no PLUGINS.md, no version bump.

estimate:
  tokens: 105000
  raw_tokens: 70000
  tasks: 2
  confidence: low          # n=0 calibration samples for a doc-only quick task

must_haves:
  truths:
    - "A reader who has never opened 260901-akh-RESEARCH.md can decide the variant, the schema key, the font policy and the review model from research/i18n-zh-hans-localization.md alone."
    - "Each of the six stages in the implementation plan can be handed to a /gsd-quick executor as written — it names its own files, its gates, its commit scope, its size, and the invocation text that starts it."
    - "The eight repo-wide prerequisites carry a file:line, the exact edit, a verification command, and the reason each must land BEFORE the first zh-Hans entry exists."
    - "The developer sees, in one list, every decision he must make before Stage 0 can begin — with a recommendation beside each."
    - "Zero plugin, script, gate, CHANGELOG or version file changed by this task."
  artifacts:
    - "research/i18n-zh-hans-localization.md — the durable reference, project rule: research goes in research/ not docs/"
    - ".planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md — the staged, executable half"
  key_links:
    - "RESEARCH §5.1 P1–P8 -> Stage 0 items, one-to-one, ids preserved so the two documents cross-reference"
    - "RESEARCH §4 width law (chars x font-size) -> Stage 1 lint rule Z6 character budget -> Stage 2 O-Chorus 6-char / 5-char cliffs"
    - "RESEARCH §6.2 reviewed enum -> Stage 0 P5 (check-i18n assertion [5]) AND Stage 5 mt -> bt -> native definition of done"
    - "research/i18n-zh-hans-localization.md <-> the implementation plan: the plan cites the research doc by path for every 'why', and repeats no measurement"
---

<objective>
The user asked two things: **how would Chinese localization be realized**, and **what is the
plan**. `260901-akh-RESEARCH.md` already answers the first with measured, file:line-cited
facts. This plan turns that one quick-task file into the two documents the user will
actually live with:

1. a durable research reference in `research/`, where this project's research belongs;
2. a staged implementation plan the user can execute later, one stage per `/gsd-quick`.

Purpose: the user leaves this task able to decide (variant, schema key, review model, pilot)
and then execute without re-deriving a single number.

Output: two markdown files. **No code.**
</objective>

<scope_fence>
**The executor writes two files and changes nothing else.**

Explicitly forbidden in this task — every one of these is *planned work for a later stage,
not work for now*:

- editing any file under `plugins/`
- editing any file under `scripts/` (including `i18n-canon.js`, `check-i18n.js`,
  `check-ui-labels.js`, `i18n-extract.js`)
- creating `scripts/i18n-zh-glossary.js`, `scripts/i18n-zh-lint.js`, or
  `scripts/i18n-zh-backtranslate.js`
- any version bump, CHANGELOG entry, `PLUGINS.md` row, build, or install

If while writing the plan the executor notices a repo fact that contradicts RESEARCH.md,
the correct action is to **write the correction into the document**, not to fix the code.
</scope_fence>

<tracer_note>
Tracer-first decomposition does not apply here: the deliverable is two documents, there is
no stack to wire end-to-end and no layer to prove. Task 1 is nonetheless ordered first
because Task 2 cites it by path.
</tracer_note>

<execution_context>
@~/.claude/gsd-core/workflows/execute-plan.md
</execution_context>

<context>
@/Users/taylorbrook/Dev/VST-development/CLAUDE.md
@/Users/taylorbrook/Dev/VST-development/.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-RESEARCH.md

Shape references — skim, do not copy wholesale:
@/Users/taylorbrook/Dev/VST-development/.planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-PLAN.md
@/Users/taylorbrook/Dev/VST-development/.planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-STAGE-N-BRIEF.md
</context>

<measured_this_session>
Verified against the tree on 2026-09-01, after RESEARCH.md was written. **Both documents
must carry these; two of them correct RESEARCH.md and the correction is the point.**

1. **`check-ui-labels.js` has SIX `['en', 'fr']` loops, not seven.** RESEARCH §5.1 P6 lists
   `598, 635, 658, 669, 703, 717, 851`. Measured: the loops are at **598, 658, 669, 703,
   717, 851**. Line 635 is inside a comment-and-control block (`const enControl = []` and
   the EN->EN spread sampling), not a language loop. Grep used:
   `grep -n "\['en', *'fr'\]" scripts/check-ui-labels.js`.

2. **`check-i18n.js` has a SECOND `reviewed`-flag site RESEARCH's P5 omits.** The tooltip
   assertion is `check-i18n.js:576-578`; the **LABELS** analogue is at
   **`check-i18n.js:623`** (`typeof ((LABELS_EARLY[k] && LABELS_EARLY[k].fr) || {}).reviewed
   !== 'boolean'`). P5 must name both, or the enum lands on tooltips and the labels table
   still demands a boolean.

3. Confirmed exactly as RESEARCH states — do not re-verify, quote them:
   - `scripts/i18n-canon.js:216` — `.then((code) => applyI18n(code === 'fr' ? 'fr' : 'en'))`
   - `scripts/i18n-canon.js:162` — `uiLanguage = LANGUAGES.includes(lang) ? lang : 'en'`
     (the guard that makes P1's fix safe) and `:166` — `document.documentElement.lang = uiLanguage`
   - `scripts/check-i18n.js:497-498` — the `LANGUAGES.join(',') === 'en,fr'` assertion [1]
   - `scripts/check-i18n.js:516, 548, 605` — the three `['en', 'fr']` sites
   - `scripts/i18n-extract.js:1252` — the `fr: { t: 'TODO', b: '', reviewed: false },` emit
   - `scripts/i18n-fr-lint.js:38-52` — T1–T7/G1/C1/F1, the rules that must NOT be reused
   - 43 `js/i18n.js` files exist; repo is on `main`, working tree clean but for this quick dir

4. **Per-plugin entry counts for the Stage 4 wave table** — this command produces them and
   was run this session (grep-approximate; within ~1-8 of RESEARCH's VM-parsed figures, so
   label the column *approx.*):
   ```bash
   for f in $(find plugins -name i18n.js | sort); do \
     p=$(echo "$f" | cut -d/ -f2); \
     n=$(grep -cE "^\s*'?[A-Za-z0-9_.-]+'?:\s*\{\s*(en|$)" "$f"); \
     echo "$n $p $f"; done | sort -rn
   ```
   Top of that list, measured: O-MicrotonalSampler 270, O-Prism 262, O-IntonationPad 199,
   O-Bells 186, O-Formant 182, O-Lyrica 167, O-Octagon 131, O-Wind 119, O-Bitrot 117,
   O-simpleGrain 115, O-simpleSampler 109, O-Contrabass 103.

5. **Repo git trap that will bite both commits:** `git commit -- <path>` takes only
   **tracked** files. Both deliverables are new. Each task must `git add <path>` first, then
   `git commit -- <path>`. Path-scope every commit; never `git add -A`, never `git commit -a`
   (CLAUDE.md, concurrent-session index race).
</measured_this_session>

<tasks>

<task type="auto">
  <name>Task 1: Promote the research into research/i18n-zh-hans-localization.md</name>
  <precondition>`.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-RESEARCH.md` exists and is the completed research output. If absent, halt — there is nothing to promote.</precondition>
  <files>research/i18n-zh-hans-localization.md</files>
  <action>
Create the durable reference. Project rule (CLAUDE.md): research documents live in
`research/`, never `docs/`.

**Frontmatter** — match the house shape used by `research/cross-platform-webview-best-practices.md`:
`title`, `created: 2026-09-01`, `last_verified: 2026-09-01`, a one-paragraph `summary`,
`domain: ui`, `type: research`, a `keywords` list (i18n, localization, chinese, zh-Hans,
cjk, webview, typography, fonts), and `stages: [3]`.

**Required `##` headings, in this order and with these exact opening strings** (a later
verification greps them):

- `## Recommendation summary` — the seven-row decision table from RESEARCH's opening,
  verbatim in substance: variant, schema key, fonts, geometry, review model, pilot,
  prerequisite gate changes.
- `## 1. Current state` — the measured inventory table (43 plugins, 3,789 entries,
  1,422 tooltip + 2,367 label, 1,917 unique short strings, 543 shared covering 64%,
  1,284 unique bodies at ~33,350 words, zero bundled webfonts, 43 identical C++ codecs,
  47 canon copies of which 43 ship).
- `## 2. Variant, schema key` — Simplified first; `zh-Hans` as BOTH the table key and the
  `document.documentElement.lang` value; why not `zh` and not `zh-CN`; the hyphen-safety
  argument (no new file, so `juce_add_binary_data` hyphen stripping does not apply).
- `## 3. Fonts, rendering, and typography` — no silent-fallback hazard because there are
  zero font files and zero `@font-face`; the declared-stack distribution; PingFang SC on
  macOS, SimSun's 12px bitmap cliff and Microsoft YaHei on Windows; the CJK-tail
  recommendation and why `:root:lang(zh-Hans)` is the wrong instrument; the minimum-legible-size
  data and the parity-font-size recommendation; wrapping/punctuation/hyphens/quotes; and the
  `line-height: normal` +30% line-box trap with its measured EN/ZH table.
- `## 4. Width geometry` — the law `zh width = charCount x font-size, exactly`, its
  inversion `maxChars = floor(cellWidthPx / fontSizePx)`, the 5/32 and 1/32 parity results,
  the five mixed-case offenders with their px deltas, and the conclusion that the French
  width-pinned-abbreviation mechanism has no Chinese analogue.
- `## 5. Schema and runtime changes` — the P1..P8 prerequisite table with file:line, the
  per-plugin edit list, and the APVTS finding (no migration needed; a zh session read by an
  older build degrades to English by existing design).
- `## 6. Translation production and review model` — the corpus sizing, the three-option
  comparison, the `reviewed: 'mt' | 'bt' | 'native'` enum with `'bt'` as the ship bar, and
  the Z1..Z7 lint ruleset with the explicit statement that the French T1-T7 and C1 must NOT
  be reused.
- `## 7. Repo-specific pitfalls` — P-1 through P-6, keeping the encoding analysis (module
  scripts always decode UTF-8; index.html is the thin margin; write the endonym as numeric
  entities) and the "zero Chinese literals in Source/" rule.
- `## 8. Rollout shape` — a short summary that points at the implementation plan by path
  rather than duplicating it.
- `## Assumptions Log` — A1..A5, kept short.
- `## Sources` — primary (in-repo file:line) and secondary (the cited URLs), unchanged.

**Apply the two corrections from `<measured_this_session>`**: in §5 P6 write six
`['en', 'fr']` loops at 598, 658, 669, 703, 717, 851 and state that line 635 is a comment
block, not a hard-code; in §5 P5 name both reviewed-flag sites, `check-i18n.js:576-578`
(tooltips) and `check-i18n.js:623` (LABELS).

**Strip quick-task scaffolding**: no "this session", no "researched by", no quick-task id in
the prose. Keep every file:line citation and every [VERIFIED]/[CITED] marker — they are the
document's value. Keep the Assumptions Log as a short section; it is honest about what was
measured on macOS/Chromium only.

Commit with `git add research/i18n-zh-hans-localization.md` first (the file is untracked and
`git commit -- <path>` would not see it), then
`git commit -- research/i18n-zh-hans-localization.md`.
  </action>
  <verify>
    <automated>node -e "const fs=require('fs');const s=fs.readFileSync('research/i18n-zh-hans-localization.md','utf8');const need=['## Recommendation summary','## 1. Current state','## 2. Variant, schema key','## 3. Fonts, rendering, and typography','## 4. Width geometry','## 5. Schema and runtime changes','## 6. Translation production and review model','## 7. Repo-specific pitfalls','## 8. Rollout shape','## Assumptions Log','## Sources'];const miss=need.filter(h=>!s.includes(h));if(miss.length){console.error('MISSING HEADING: '+miss.join(' | '));process.exit(1);}for(const k of ['title:','created:','last_verified:','summary:','keywords:']){if(!s.includes(k)){console.error('MISSING FRONTMATTER KEY: '+k);process.exit(1);}}for(const a of ['i18n-canon.js:216','check-i18n.js:623','i18n-extract.js:1252','zh-Hans','maxChars']){if(!s.includes(a)){console.error('MISSING ANCHOR: '+a);process.exit(1);}}if(s.length<12000){console.error('TOO THIN: '+s.length+' bytes');process.exit(1);}console.log('research doc OK, '+s.length+' bytes');"</automated>
  </verify>
  <done>`research/i18n-zh-hans-localization.md` exists, carries all eleven required headings and the house frontmatter keys, cites `i18n-canon.js:216` / `check-i18n.js:623` / `i18n-extract.js:1252`, is over 12 KB, and is committed path-scoped. Nothing under `plugins/` or `scripts/` changed.</done>
</task>

<task type="auto">
  <name>Task 2: Write the staged 260901-akh-IMPLEMENTATION-PLAN.md</name>
  <files>.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md</files>
  <action>
This is the primary user-facing artifact. Write it so each stage can be pasted to an
executor as-is. Cite `research/i18n-zh-hans-localization.md` for every "why"; do not repeat
its measurements beyond the numbers a stage needs to act.

**Open with `## Decisions to confirm before Stage 0`** — a table of the choices that are the
user's, not the executor's, each with a recommendation and the consequence of the other
branch: (a) Simplified first vs Traditional first; (b) schema key `zh-Hans` vs `zh`/`zh-CN`;
(c) the `reviewed: 'mt' | 'bt' | 'native'` review model; (d) whether to SHIP at `'bt'` or
hold the release until a native reader signs off — name this as the one decision that gates
whether the work has an end; (e) pilot = O-Chorus; (f) the four unserved
`plugins/*/.planning/i18n-index-draft.html` drafts — sync or delete.

**Then six stages. Every stage carries, in this order:** a one-line goal, the numbered work
items, then five bold fields — `**Files touched:**`, `**Gates:**`, `**Commit scope:**`,
`**Size:**`, `**Invocation:**`. The invocation is copy-paste `/gsd-quick` text naming the
stage and its exit criterion. Commit scope is a literal path-scoped `git commit --` line per
CLAUDE.md; where a file is new, show the `git add` that must precede it.

- `## Stage 0 — Repo-wide prerequisites` (~8 items, 45 files, one or two commits). One row
  per prerequisite **P1..P8**, keeping RESEARCH's ids so the two documents cross-reference.
  Each row: file:line, the exact edit, the verification command, and the ordering constraint
  — why it must land before the first zh-Hans entry. Call out that P1+P2 is a **43-file
  same-commit canon sweep** (`i18n-canon.js:216` plus the 43 shipping copies byte-compared by
  `check-i18n.js` assertion 6), that P1's correct form is `applyI18n(code)` relying on the
  guard already at `i18n-canon.js:162` so a fourth language never needs another sweep, and
  that the sweep targets are `grep -rl "applyI18n(code === 'fr'" plugins` minus the four
  `.planning/i18n-index-draft.html` drafts. Use the corrected P5 (two sites: 576-578 and 623)
  and P6 (six loops, 635 is a comment block). Exit criterion: `check-i18n` ALL PASS on 43/43
  with the tables still at two languages, plus a `check-ui-labels` dry run proving the loops
  read `LANGUAGES` rather than a literal.
- `## Stage 1 — zh-Hans glossary and lint` (no plugin touched). `scripts/i18n-zh-glossary.js`
  over the 543 shared strings covering 64% of occurrences, each term carrying a character
  budget; `scripts/i18n-zh-lint.js` with Z1..Z7 plus G1-equivalent conformance and the
  `reviewed` enum check, and explicitly WITHOUT the French T1-T7 and C1; and
  `scripts/i18n-zh-backtranslate.js` emitting `en -> zh -> en'` triples. State the lifecycle:
  all three ship as **reports first**, promoted to gates only once the pilot is at zero
  findings, exactly as `i18n-fr-lint.js` was. State the ordering law plainly — the French
  rollout produced 267 divergent renderings before a glossary existed; no per-plugin dispatch
  begins until the glossary is settled.
- `## Stage 2 — Pilot: O-Chorus` (29 entries). The per-plugin edit list: `i18n.js` LANGUAGES
  plus a `'zh-Hans'` value on every key; the `<option value="zh-Hans">` written as numeric
  entities `&#31616;&#20307;&#20013;&#25991;`; the CJK font tail appended to the ~10 stacks
  the localized nodes resolve through; `PluginProcessor.h` `languageCode`/`languageIndex`
  third code, ASCII only; version bump plus CHANGELOG; `./scripts/build-and-install.sh`;
  `check-i18n`, `check-ui-labels` zh arm, `i18n-zh-lint`. Convert O-Chorus's own recorded px
  cliffs into character budgets via the width law — 62px wrap cliff / 10px = 6 chars, 50px
  gate cliff / 10px = 5 chars. Include the `line-height: normal` audit as an explicit step,
  since assertion 7's zh arm is what names the offending nodes. Exit criterion: zero geometry
  moved, zero lint findings, back-translation read.
- `## Stage 3 — Hard-case wave` (3 plugins, ~517 entries). O-Octagon (131: all-caps pinned
  speaker labels, its own `ui_layout_check`, and the index.html whose `<meta charset>` sits at
  byte 2920), O-Bitrot (117: inline-module controller, its own clamp gate), O-MicrotonalSampler
  (270: largest table, `Resources/ui/` root, and the English pluralization inlined in its JS).
  Say why each is hard and what it would teach before 39 plugins carry the pattern.
- `## Stage 4 — Volume waves` (~39 plugins). Propose waves of 5-6, grouped so each wave mixes
  one heavy plugin with several light ones, and note the UI-root split (33 under
  `Source/ui/public/js/`, 10 under `Resources/ui/js/` — O-Bassoon, O-Bells, O-Bowed,
  O-FreqPulse, O-Lyrica, O-MicrotonalSampler, O-Orbit, O-Reed, O-SpectralShaper, O-Wind).
  Include a per-plugin entry-count table using the command in the plan's
  `<measured_this_session>` block, labelling the column *approx.* Note the corpus skew: the
  top 8 hold 1,530 of 3,789 entries.
- `## Stage 5 — Repo-wide QA and review pass`. `i18n-zh-lint` to 0 repo-wide;
  back-translation reviewed on 43/43; `boot-all-uis` 43/43 clean; `check-ui-labels` zh arm
  0 geometry moved; `auval` PASS x43. Then the part that matters most: define **done** given
  the developer cannot read Chinese — `'mt'` fails the ship gate, `'bt'` is the ship bar and
  means the developer read an independent back-translation against the English source,
  `'native'` stays open and is not a blocker. Say plainly that shipping at `'bt'` is a
  *disclosed* quality level, which is the entire design intent of the flag.

**Close with two short sections:** a risk list (what could still go wrong: WKWebView
line-height re-measurement per assumption A5, the sub-9px legibility tier as a disclosed
limitation, O-MultiBandCompressor as the whitespace outlier any sed-based sweep skips) and a
running total of effort by stage.

Commit with `git add` on the new file first, then
`git commit -- .planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md`.
This task owns its own commit; the orchestrator's docs commit does not need to stage it.
  </action>
  <verify>
    <automated>node -e "const fs=require('fs');const p='.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md';const s=fs.readFileSync(p,'utf8');const need=['## Decisions to confirm before Stage 0','## Stage 0','## Stage 1','## Stage 2','## Stage 3','## Stage 4','## Stage 5'];const miss=need.filter(h=>!s.includes(h));if(miss.length){console.error('MISSING SECTION: '+miss.join(' | '));process.exit(1);}for(const f of ['**Files touched:**','**Gates:**','**Commit scope:**','**Size:**','**Invocation:**']){const n=s.split(f).length-1;if(n<6){console.error('FIELD '+f+' appears '+n+' times, need >=6 (one per stage)');process.exit(1);}}for(const k of ['P1','P2','P3','P4','P5','P6','P7','P8']){if(!s.includes(k)){console.error('MISSING PREREQ ID: '+k);process.exit(1);}}for(const a of ['i18n-canon.js:216','check-i18n.js:623','check-i18n.js:497','i18n-extract.js:1252','research/i18n-zh-hans-localization.md','git commit --','O-Chorus','zh-Hans']){if(!s.includes(a)){console.error('MISSING ANCHOR: '+a);process.exit(1);}}if(s.length<14000){console.error('TOO THIN: '+s.length+' bytes');process.exit(1);}console.log('implementation plan OK, '+s.length+' bytes');"</automated>
  </verify>
  <done>The implementation plan exists with all seven required sections, all eight prerequisite ids P1..P8, at least six occurrences of each of the five per-stage fields, the corrected `check-i18n.js:623` anchor, a path-scoped `git commit --` line, a citation of the research doc by path, and is over 14 KB. Committed path-scoped.</done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| none crossed | Documentation-only task. No input parsed, no network call, no package install, no executable shipped. |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-akh-01 | Tampering | shared `.git/index` — a concurrent session's staging joining this task's commit | medium | mitigate | Both commits are path-scoped (`git add <one path>` then `git commit -- <same path>`); `git branch --show-current` and `git status --short` re-checked immediately before each commit, per CLAUDE.md |
| T-akh-02 | Tampering | scope creep into `plugins/` or `scripts/` | medium | mitigate | `<scope_fence>` enumerates the forbidden paths; the phase verification below asserts `git status --porcelain` shows only the two allowed paths |
| T-akh-03 | Information disclosure | none — no secret, key, or credential is read or written | low | accept | Nothing to disclose |
| T-akh-SC | Tampering | npm/pip/cargo installs | n/a | accept | **No package installs in this task.** No RESEARCH Package Legitimacy Audit is required because no package-manager command is run. Stage 1 of the *implementation plan* may add npm dependencies later — that stage, not this one, carries the legitimacy gate |
</threat_model>

<verification>
Run from the repo root after both tasks.

1. Both files exist and pass their own structural checks (the two `<automated>` commands above).

2. **Scope fence held** — nothing outside the two allowed paths was touched:
   ```bash
   git status --porcelain | grep -vE '(research/i18n-zh-hans-localization\.md|\.planning/quick/260901-akh-research-and-plan-chinese-localization-a/)' | wc -l
   # must print 0
   ```

3. **The gates are untouched** — the i18n tooling is exactly as it was, because Stage 0 has
   not run yet. Baseline is the HEAD this plan was written against, `9eb8564b`, so the check
   is independent of how many commits the two tasks produced:
   ```bash
   git diff --stat 9eb8564b..HEAD -- scripts plugins | wc -l
   # must print 0
   ```

4. **Location check before either commit** (CLAUDE.md, concurrent-session index race):
   ```bash
   git branch --show-current   # main
   git status --short
   ```
</verification>

<success_criteria>
- `research/i18n-zh-hans-localization.md` reads as a standalone reference: a newcomer can
  answer variant, schema key, font policy, geometry law and review model without opening the
  quick-task directory.
- `260901-akh-IMPLEMENTATION-PLAN.md` names six stages, each with files, gates, commit scope,
  size and a copy-paste `/gsd-quick` invocation.
- The eight prerequisites appear with file:line, exact edit, verification command and
  ordering constraint — and carry the two corrections measured in this plan.
- The decisions list makes clear which single choice (ship at `'bt'` or wait for a native
  reader) determines whether the work has an end.
- `git status --porcelain` shows only `research/` and the quick directory. Zero plugins, zero
  scripts, zero versions, zero builds.
</success_criteria>

<output>
Two committed files:
- `/Users/taylorbrook/Dev/VST-development/research/i18n-zh-hans-localization.md`
- `/Users/taylorbrook/Dev/VST-development/.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md`

Then write `260901-akh-SUMMARY.md` in the quick directory per the quick-task convention.
</output>
