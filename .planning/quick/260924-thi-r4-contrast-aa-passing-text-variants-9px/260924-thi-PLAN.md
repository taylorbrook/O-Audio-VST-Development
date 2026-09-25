---
phase: quick-260924-thi
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - scripts/measure-ui.js
  - .claude/aesthetics/ouaricon-naturalist-001/aesthetic.md
  - scripts/measure-ui-README.md
autonomous: true
requirements: [R4]
source: .planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md (R4, §2.2, A.4, Appendix B step 8)

estimate:
  tokens: 140000
  raw_tokens: 140000
  tasks: 3
  confidence: low

must_haves:
  truths:
    - "aesthetic.md names four AA text variants, each at or above 4.5:1 when recomputed from the file's own hex values: #7A654B walnut text on #F5E6D3 (4.52), #715D45 walnut text on #EBD9C7 (4.56), #55703E sage text on #F5E6D3 (4.54), and the planner-derived #4E6839 sage text on #EBD9C7 (4.54)"
    - "The original colours #8B7355, #6B8E4E and #8BA870 are still in aesthetic.md for borders, fills and decoration (at least 16, 4 and 3 occurrences). The Warm brown bullet and the --brown-border comment no longer list text among walnut's uses, and both point text at the AA variants"
    - "aesthetic.md sets a 9px text floor, and the Implementation Checklist asks for the floor and a --contrast run"
    - "`node scripts/measure-ui.js --plugin <P> --contrast` prints per language: text-node count, below-AA count and %, count under the 9px floor, count over a background-image, and median/min ratio. Each node's ratio lands in its stdout JSON row as `ct`. The exit code is 0 whatever it finds"
    - "On the scratch fixture, the contrast report prints exactly `contrast: 6 finding(s)` and `en: 13 text node(s), 5 below AA (38.5%), 1 under 9px floor, 1 over background-image, ratio median 3.66 / min 2.69`. That proves layer compositing, the opacity product, the large-text rule, the floor, and the transparent skip against hand-computed values"
    - "Without --contrast, stdout and stderr are byte-identical to the pre-edit script on the O-TextureForge HEAD snapshot, both in fonts mode and in `--mode box --report all`. `--report all` still runs exactly the four original screens"
    - "SUMMARY.md records the contrast baseline for O-TextureForge, O-MicrotonalSampler, O-Prism and O-ReverseDelay, measured on a `git archive` snapshot at a recorded sha. Its numbers agree with an independent recompute from the saved rows"
    - "None of this task's commits touches a path under plugins/"
  artifacts:
    - path: scripts/measure-ui.js
      provides: "Opt-in --contrast: pageProbe `ct` field (composited fg/bg, WCAG ratio, need, large, fs, fw, img, ah, skip) + pure `contrast` screen (report-only) + CLI wiring; default output unchanged"
      contains: "--contrast"
    - path: .claude/aesthetics/ouaricon-naturalist-001/aesthetic.md
      provides: "AA text-variant pair table, originals re-scoped to borders/decoration, 9px text floor, --text-walnut/--text-sage CSS vars, checklist items"
      contains: "#7A654B"
    - path: scripts/measure-ui-README.md
      provides: "Contrast report section, Control 3 (fixture positive control recipe + expected lines), limitations, four-plugin reference reading"
      contains: "--contrast"
  key_links:
    - from: "scripts/measure-ui.js CLI (--contrast / --report contrast)"
      to: "pageProbe({ contrast: true }) -> rows[].ct -> SCREENS.contrast via runScreen"
      via: "wantContrast flag passed into page.evaluate(pageProbe, {...}); emit() appends 'contrast' to the screen list"
      pattern: "wantContrast"
    - from: "SCREENS.contrast"
      to: "runScreen SKIPPED line"
      via: "per-screen `hint` ('needs --contrast'); the four original screens keep the 'needs --mode box' text byte-identical"
      pattern: "needs --contrast"
    - from: ".claude/aesthetics/ouaricon-naturalist-001/aesthetic.md \"Example Color Codes\""
      to: ".claude/skills/ui-template-library/references/apply-operation.md (CSS variable insertion reads this section for new plugins)"
      via: "new --text-walnut / --text-walnut-mid / --text-sage / --text-sage-mid variables"
      pattern: "--text-walnut"
---

<objective>
Implement **R4** from the UI design review (260924-nho), template half plus measurement only.

1. **Fix the palette at the source.** Add AA-passing text variants to the brand template `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`. Keep the original colours for borders, fills and decoration, and set a 9px text floor.
2. **Make contrast measurable.** Add an opt-in, report-only `--contrast` report to `scripts/measure-ui.js`. It gives a WCAG ratio for each visible text node against its nearest opaque background, the % below AA, and the count under the 9px floor.
3. **Record the baseline** on O-TextureForge, O-MicrotonalSampler, O-Prism and O-ReverseDelay in SUMMARY.md.

**No plugin edits.** Applying the variants per plugin is R4's second half and is out of scope here. It is a visible colour shift that the review says to do per plugin, with before/after screenshots.

Purpose: legibility is the most visible UX defect the review found. The template's own text colours fail AA at its prescribed sizes. On screen, 5–75% of text is below AA in the sampled plugins, and no tool in the repo measures contrast (review §4.2: "0 hits for `contrast`"). This plan fixes the source and gives every later per-plugin fix a measured before-number.

Output: one modified script, one modified template, one modified README, and a SUMMARY.md baseline table.

Decisions this plan fixes. There is no CONTEXT.md, so these are planner decisions from live observation at HEAD `f9111a53`:
- **The three user-named variants are verified and kept as given.** Recomputed at planning time (WCAG 2.x, sRGB linearisation): `#7A654B` on `#F5E6D3` = 4.522, `#715D45` on `#EBD9C7` = 4.557, `#55703E` on `#F5E6D3` = 4.543. All three pass 4.5:1, so no substitution was needed. The review's k-scaling method (Appendix B step 8) reproduces each one exactly: k = 0.88, 0.81 and 0.79.
- **A fourth variant is added: `#4E6839`, sage text on `#EBD9C7` (4.544, k = 0.73).** The template names `#EBD9C7` as a primary background, and `#55703E` reads only 4.050 there, which fails. Without this row the template would still prescribe a failing pair. The walnut pair has the same trap: `#7A654B` on `#EBD9C7` = 4.032. The pair table must therefore bind each variant to its background. `#715D45` is also 5.111 on `#F5E6D3`, so it is the one walnut that is safe on either paper tone.
- **Accent panel `#D4C4B0`: no new variant.** `#715D45` there reads 3.674, which fails. The existing `#5C4033` reads 5.510, which passes, so the template points text on that panel at `#5C4033` or darker.
- **Button text is already fine.** `#2C3E10` reads 7.69 on the default green tint and 5.02 on the active tint (composited over `#F5E6D3`). It is not changed.
- **New CSS variable names avoid `--text-muted`.** Three plugins already use that name (O-Detune, O-MicrotonalSampler, O-SimpleReverb). The new names are `--text-walnut`, `--text-walnut-mid`, `--text-sage` and `--text-sage-mid`. No plugin uses any of these.
- **`--contrast` is opt-in and outside `--report all`.** Putting it in `all` would add a SKIPPED line to every existing `--report all` run and break byte-identity with the pre-edit output. `--report contrast` is accepted as an alias that also turns on collection.
- **Same methodology as the review's A.4.** The report uses visible elements that own a text node, own colour × alpha × opacity product, and the nearest opaque ancestor's background-color with semi-transparent layers composited. `background-image` is flagged, not sampled. This keeps the numbers comparable with 75% / 69% / 44% / 5%.
- **The fixture positive control stays in scratch, not in the repo.** Its exact recipe and expected lines go into the README as Control 3, beside Controls 1–2, which are also command recipes. Committing a fake plugin tree would put an `O-*` folder where future repo-wide globs could pick it up.
</objective>

<execution_context>
@~/.claude/gsd-core/workflows/execute-plan.md
@~/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@CLAUDE.md
@.planning/STATE.md
@scripts/measure-ui.js
@.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md

Facts established during planning (HEAD `f9111a5332b9374f21ab28c11e81566f1b6466a9`). Re-derive them; do not trust them blindly:

- **SCR.** Set it at the top of EVERY Bash call to `<your session scratchpad>/thi`, the session-specific scratchpad path the harness gives you. Shell state does not persist between calls. Never use `/tmp`, and never write inside the repo. The verify blocks open with `SCR="${SCR:?…}"`, so an unset SCR fails loudly instead of creating a literal relative directory inside the repo. Every scratch artefact below (snapshot, fixture, before/after captures, cross-check script) lives under `$SCR`.
- **HEAD snapshot.** `git archive HEAD -- plugins/O-TextureForge plugins/O-MicrotonalSampler plugins/O-Prism plugins/O-ReverseDelay modules | tar -x -C "$SCR/snap"` works as `--root "$SCR/snap"`. The stub is read from the live `scripts/ui-stub/` via `__dirname`. `modules/` is needed because O-MicrotonalSampler direct-embeds module files. Measuring the working tree instead would measure other sessions' uncommitted work: O-ReverseDelay has 14 dirty UI/editor files, and O-Prism has 7 dirty files including CMakeLists.txt.
- **Frames at HEAD** (from `setSize`): O-TextureForge 900×600, O-MicrotonalSampler 900×640, O-Prism 1200×800, O-ReverseDelay **940×693**. The review's 940×768 is stale. All four have `tests/i18n-states.json`. O-ReverseDelay has its own stub; the other three use the generic one.
- **Runtime and determinism.** `measure-ui.js --plugin O-Prism --root <snap>` took about 19 s (7008 rows / 2336 DOM keys). O-TextureForge `--mode box` gave byte-identical stdout on two consecutive runs (sha `169a970b0a81…`).
- **Fixture recipe, verified at planning with the unmodified script.** A tree `<root>/plugins/O-Fixture/` needs three files: `CMakeLists.txt` with `juce_add_binary_data(O-Fixture_WebUI SOURCES` listing `Source/ui/public/index.html` and `Source/ui/public/js/i18n.js`; `Source/PluginEditor.cpp` containing `setSize (400, 300);`; and `Source/ui/public/js/i18n.js` containing `export const LANGUAGES = ['en'];`. Add an empty `<root>/modules/` directory. `node scripts/measure-ui.js --plugin O-Fixture --root <root> --verbose` then resolves `ui=Source/ui/public frame=400x300 languages en`.
- **measure-ui.js structure.** `pageProbe({ mode, sel, hanSrc })` builds a `base` row: key, id, vis, han, own (sliced to 40), kids, ffAttr. It then adds mode fields. Note that inside pageProbe the local `bb` is the bounding rect, but in box-mode rows `bb` means border-bottom width, so do not reuse that name. The accumulator ORs `vis` and `han` and copies the other fields from the last visible state (design note 3). `runScreen` checks `sc.needs` against the first row and prints the fixed line `<name>: SKIPPED — needs --mode box (field <f> not present)`. `emit()` prints JSON to stdout, then runs screens. `module.exports` already exports `pageProbe, SCREENS, SCREEN_ORDER`. Nothing in the repo `require`s measure-ui.js.
- **Template consumers.** `.claude/skills/ui-template-library/references/apply-operation.md` interprets aesthetic.md as prose and takes CSS variables from the "Example Color Codes" section. SKILL.md requires the same section headers in the same order. So the edits must keep every `## ` header and its order, and add only a `### ` sub-section. `metadata.json` stays untouched.
- **Occurrences in aesthetic.md at HEAD.** `#8B7355` ×16, `#6B8E4E` ×4, `#8BA870` ×3. The two lines that list text among walnut's uses are the Earth Tone Accents walnut bullet (~line 51) and the `--brown-border` comment (~line 607). There is no 9px floor statement anywhere.
- **Concurrent sessions.** Other sessions have uncommitted work in `PLUGINS.md`, `plugins/O-Formant/**`, `plugins/O-Prism/**` and `plugins/O-ReverseDelay/**`, plus an untracked `plugins/O-Prism/tests/distortion_alias_check.cpp`. Never stage, restore, stash, check out or commit anything there. Every commit is path-scoped (`git commit -m … -- <path>`). Run `git branch --show-current` and `git status --short` immediately before each commit, not once at the start. This is trunk-based on `main`, so do not branch.
- **No plugin build.** No plugin binary changes, so do not run the build-and-install / AU cache sequence from CLAUDE.md.
</context>

<tasks>

<task type="tracer">
  <name>Task 1: End-to-end opt-in --contrast in measure-ui.js, proven on a scratch fixture with hand-computed ratios</name>
  <files>scripts/measure-ui.js</files>
  <read_first>
    - scripts/measure-ui.js (whole file: header design notes, pageProbe, measure() accumulator, SCREENS, runScreen, CLI/emit)
    - scripts/serve-ui.js lines 377-460 (buildRoot: how --root fixture trees are resolved), only if the fixture misbehaves
  </read_first>
  <action>
**Step 0: capture the pre-edit baseline BEFORE touching the file.**
- Create `$SCR/snap`, extract the HEAD snapshot into it (see context), and write `git rev-parse HEAD` to `$SCR/head.sha`.
- Using the UNMODIFIED script on O-TextureForge with `--root "$SCR/snap"`, capture stdout and stderr of two invocations:
  - (i) the default: `--plugin O-TextureForge --root "$SCR/snap"` → `$SCR/before-fonts.json` / `.err`;
  - (ii) `--mode box --report all` → `$SCR/before-box.json` / `.err`.
- Run each invocation a second time and `cmp` the two runs. If the runs differ, the byte-identity check below falls back to comparing the per-row key sets and row counts. Record whichever check applies.

**Step 1: page probe.** Add an optional `contrast` member to pageProbe's destructured options (default falsy).
- When it is falsy, the row object must be exactly what it is today. Do not add a `ct` key at all, and leave the `base` fields and their order untouched.
- When it is truthy, set `ct` on every row, after the existing mode branch:
  - `null` unless the node is `vis` and its trimmed own text (before the 40-char slice) is non-empty.
  - Otherwise an object with these fields:
    - `fg`, `bg`: composited colours as uppercase `#RRGGBB`.
    - `ratio`: full-precision float, computed FROM the rounded 8-bit fg/bg values, so that an independent recompute from the two hex strings reproduces it exactly.
    - `need`: 4.5 or 3.
    - `large`: bool.
    - `fs`: computed font-size in px, a number.
    - `fw`: numeric computed font-weight.
    - `img`: bool.
    - `ah`: bool, true when the node or an ancestor has `aria-hidden="true"`.
    - `skip`: `null`, `'transparent'` or `'unparsed'`. When skip is non-null, fg and ratio are null.

How each field is computed:
- **Colour source.** HTML elements use the computed `color`. SVG elements (`instanceof SVGElement`) use the computed `fill`; `none` or a `url(...)` value counts as unparsed.
- **Parsing.** A regex handles `rgb()` and `rgba()` in both the comma syntax and the space/slash syntax. Chromium serialises hex, named and sRGB-authored colours this way. Anything the regex does not match falls back to ONE reused 1×1 2d canvas: set `fillStyle`, `fillRect`, read `getImageData`. If that also fails, the node is `'unparsed'`.
- **Foreground alpha.** Effective alpha = colour alpha × the product of computed `opacity` over the node and every ancestor up to documentElement. Below 0.01, skip the node as `'transparent'`.
- **Background.** Walk from the node itself up `parentElement` to documentElement:
  - background-color alpha ≥ 0.99 is the opaque base; stop there;
  - alpha strictly between 0 and 0.99 is pushed as a layer;
  - any element on this chain, up to and including the base, whose computed `background-image` is not `none` sets `img = true`.
  - If no base is found, use `#FFFFFF`, Chromium's canvas default. Say so in the header note.
  - Composite the layers outermost-first over the base (source-over: a·layer + (1−a)·below), then round to 8-bit to get `bg`.
- **Foreground composite.** α·fg + (1−α)·bg, rounded to 8-bit, gives `fg`.
- **Ratio.** WCAG 2.x relative luminance: channel c/255; c ≤ 0.04045 → c/12.92, else ((c+0.055)/1.055)^2.4; L = 0.2126R + 0.7152G + 0.0722B; ratio = (Lmax+0.05)/(Lmin+0.05).
- **Large text and threshold.** `large` = fs ≥ 24, or (fs ≥ 18.66 and fw ≥ 700). `need` = 3 if large, else 4.5. These are the user-specified thresholds.

**Step 2: the screen.** Add `contrast` to SCREENS as a pure function over rows: `needs: ['ct']`, plus a new optional per-screen `hint: 'needs --contrast'`.
- In runScreen, the SKIPPED line uses `sc.hint`, falling back to today's literal. The four existing screens must print byte-identical SKIPPED text.
- Do NOT add `contrast` to SCREEN_ORDER. `--report all` must still run exactly the four original screens.
- **Counted rows:** `vis && ct && ct.skip === null`.
  - Below AA: `ratio < need`, compared on the unrounded ratio. 4.499 fails.
  - Under floor: `fs < 9`.
- **Output.** The count line is `contrast: <N> finding(s)`, where N = counted rows that are below AA OR under the floor (the union, across all languages). Then these notes, each indented two spaces:
  - one line per language, in first-seen order: `<lang>: <T> text node(s), <B> below AA (<pct>%), <F> under 9px floor, <I> over background-image, ratio median <m> / min <n>`
    - pct to 1 decimal, `n/a` when T = 0
    - median and min to 2 decimals
  - `<A> of the below-AA node(s) sit under aria-hidden (decorative) — still counted`
  - `skipped: <x> transparent, <y> unparsed colour`
  - `thresholds: 4.5:1 normal text, 3:1 large (>= 24px, or >= 18.66px at weight >= 700); floor 9px`
  - `background: nearest opaque ancestor background-color, semi-transparent layers composited; background-image is NOT sampled (flagged only)`
- **Findings.** Sorted by ratio ascending. Format: `<lang>  <key>  (<id>)  "<own>"  ratio=<2dp> need=<need> fg=<hex> bg=<hex> fs=<px>px`, followed by tags ` [AA]`, ` [<9px]`, ` [img]`, ` [aria-hidden]` as they apply.

**Step 3: CLI.** Add `wantContrast` = `--contrast` present OR `--report` equals `contrast`.
- Pass `contrast: wantContrast` into the existing `page.evaluate(pageProbe, {...})` call.
- In `emit()`, build the screen list as today (`all` → SCREEN_ORDER, otherwise `[report]`, and none when there is no --report). If wantContrast, append `contrast` once. The report then works with `--from` as well.
- Exit codes are unchanged: contrast findings never change the exit code. This is report-only.
- In USAGE, add a `--contrast` line and note that `contrast` is opt-in and not part of `all`.

**Step 4: header comment.** Add a `── THE CONTRAST REPORT (opt-in) ──` block after THE FOUR SCREENS. It covers:
- what the report measures;
- R4 provenance (260924-nho §2.2 / A.4 method);
- why it is opt-in and outside `all`;
- the white-canvas fallback;
- disclosed limitations:
  - image backgrounds are flagged, not sampled;
  - absolutely positioned overlays and stacked siblings that are not ancestors are invisible to the walk;
  - text in pseudo-element `content`, input values and closed-select captions is not counted;
  - computed font-size ignores CSS transforms, `zoom` and SVG viewBox scaling;
  - `filter: opacity()` is ignored;
  - opacity is applied to the foreground only, not to background layers;
  - each node's values come from the last state in which it was visible (design note 3).

**Step 5: fixture positive control.** Build it in `$SCR/fix` using the context recipe (never in the repo). `index.html` has body `margin:0`, background `#F5E6D3`, font-size 10px, and `p { margin: 0 }`. Elements, with expected results:

| id | markup | expected |
|---|---|---|
| a | p, `#8B7355` | 3.66, need 4.5, below |
| b | p, `#7A654B` | 4.52, pass |
| c | p inside a div with background `#EBD9C7`, colour `#715D45` | 4.56, pass |
| d | p, `#55703E` | 4.54, pass |
| e | p, `#8B7355` at 26px | 3.66, need 3, pass |
| f | p, `#6B8E4E` at 19px weight 700 | 3.06, need 3, pass |
| g | p, `#6B8E4E` at 19px weight 400 | 3.06, need 4.5, below |
| h | p, `#3C2F2F` at 8px | 10.45, pass, under floor |
| i | p, colour `#000000`, inside a div (background `rgba(0,0,0,0.5)`) inside a div (background `#FFFFFF`) | bg `#808080`, 5.32 |
| j | p, colour `rgba(60,47,47,0.5)` | fg `#998B81`, 2.69, below |
| k | p, `#3C2F2F`, inside a div with opacity 0.5 | 2.69, below |
| m | p, `#3C2F2F`, inside a div with background-color `#F5E6D3` and background-image `linear-gradient(#000,#000)` | 10.45, img |
| n | span with `aria-hidden="true"`, colour `#8B7355`, text `&#10086;` | 3.66, below, ah |
| o | p, `#3C2F2F`, opacity 0 | skip `'transparent'` |
| p | p, `display:none` | not counted |
| q | p, `visibility:hidden` | not counted |

Then run `node scripts/measure-ui.js --plugin O-Fixture --root "$SCR/fix" --contrast > "$SCR/fix.json" 2> "$SCR/fix.err"`. Every value in the verify block must match.

**Step 6: commit.**
- Run `git branch --show-current` (it must print `main`) and `git status --short` immediately before committing.
- Commit with `git commit -m "feat(quick-260924-thi): measure-ui --contrast — opt-in WCAG AA report (report-only)" -- scripts/measure-ui.js`.
  </action>
  <verify>
    <automated>SCR="${SCR:?prefix this call with SCR=<your session scratchpad>/thi}"; cd /Users/taylorbrook/Dev/VST-development && node --check scripts/measure-ui.js && node scripts/measure-ui.js --plugin O-Fixture --root "$SCR/fix" --contrast > "$SCR/fix.json" 2> "$SCR/fix.err" && echo "fixture run exit=0" && grep -Fx 'contrast: 6 finding(s)' "$SCR/fix.err" && grep -F '  en: 13 text node(s), 5 below AA (38.5%), 1 under 9px floor, 1 over background-image, ratio median 3.66 / min 2.69' "$SCR/fix.err" && grep -F 'skipped: 1 transparent, 0 unparsed colour' "$SCR/fix.err" && grep -F '1 of the below-AA node(s) sit under aria-hidden' "$SCR/fix.err" && node -e 'const r=require(process.argv[1]).filter(x=>x.vis&&x.ct);const g=id=>(r.find(x=>x.id===id)||{}).ct;const E={"#a":[3.66,4.5],"#b":[4.52,4.5],"#c":[4.56,4.5],"#d":[4.54,4.5],"#e":[3.66,3],"#f":[3.06,3],"#g":[3.06,4.5],"#h":[10.45,4.5],"#i":[5.32,4.5],"#j":[2.69,4.5],"#k":[2.69,4.5],"#m":[10.45,4.5],"#n":[3.66,4.5]};let bad=0;for(const[id,[rt,nd]]of Object.entries(E)){const c=g(id);if(!c||c.skip!==null||Math.abs(c.ratio-rt)>0.01||c.need!==nd){console.error("BAD",id,JSON.stringify(c));bad++}}if(!(g("#m")&&g("#m").img&&g("#n").ah&&g("#o").skip==="transparent"&&g("#i").bg==="#808080"&&g("#j").fg==="#998B81"))bad++;if(r.some(x=>x.id==="#p"||x.id==="#q"))bad++;console.log(bad?"FIXTURE FAIL":"FIXTURE OK");process.exit(bad?1:0)' "$SCR/fix.json" && node scripts/measure-ui.js --plugin O-TextureForge --root "$SCR/snap" > "$SCR/after-fonts.json" 2> "$SCR/after-fonts.err" && node scripts/measure-ui.js --plugin O-TextureForge --root "$SCR/snap" --mode box --report all > "$SCR/after-box.json" 2> "$SCR/after-box.err" && cmp "$SCR/before-fonts.json" "$SCR/after-fonts.json" && cmp "$SCR/before-box.json" "$SCR/after-box.json" && diff "$SCR/before-fonts.err" "$SCR/after-fonts.err" && diff "$SCR/before-box.err" "$SCR/after-box.err" && node scripts/measure-ui.js --report contrast --from "$SCR/before-box.json" 2>&1 >/dev/null | grep -F 'contrast: SKIPPED — needs --contrast (field ct not present)' && node -e 'const m=require("./scripts/measure-ui.js");if(m.SCREEN_ORDER.length!==4||m.SCREEN_ORDER.includes("contrast")||!m.SCREENS.contrast)process.exit(1);console.log("exports OK")'</automated>
  </verify>
  <done>
- `--contrast` runs end to end on the fixture with exit 0. All 13 fixture ratios match the hand-computed values within ±0.01, and the count, per-language, skipped and aria-hidden lines match exactly.
- Without `--contrast`, the O-TextureForge stdout and stderr are byte-identical to the pre-edit capture in both modes. If Step 0 found the pre-edit runs themselves non-deterministic, the key-set and row-count comparison is used instead, and that fact is recorded.
- `--report all` is unchanged. `--report contrast --from <rows without ct>` prints the SKIPPED hint.
- The change is committed path-scoped to `scripts/measure-ui.js` only.
  </done>
</task>

<task type="auto">
  <name>Task 2: AA text variants, originals re-scoped to borders/decoration, and a 9px text floor in the naturalist brand template</name>
  <files>.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md</files>
  <read_first>
    - .claude/aesthetics/ouaricon-naturalist-001/aesthetic.md lines 36-122 (Color System, Typography) and 597-661 (Example Color Codes, Implementation Checklist)
  </read_first>
  <action>
Before editing, save `grep '^## '` of the file to `$SCR/headers.before`. Then make these edits with Edit, scoped. Do not Write the whole file.

1. **Earth Tone Accents (~lines 49–51).** Re-scope the three originals; keep every hex value.
   - Moss `#8BA870`: active-state fills and botanical references, not text (2.16:1 on `#F5E6D3`).
   - Deeper green `#6B8E4E`: hover fills, emphasis borders and decoration. Sage TEXT uses `#55703E` / `#4E6839`, because the original is 3.06:1.
   - Walnut `#8B7355`: keep the bullet's `- Warm brown: #8B7355 (walnut/oak)` prefix, and rewrite the rest so its uses are borders, structural elements and decoration only. Remove text from that list, and point walnut text at the AA variants below (3.66:1 on `#F5E6D3`, 3.26:1 on `#EBD9C7`).
2. **Text Colors block (~lines 54–58).** Keep primary `#3C2F2F` and secondary `#5C4033`.
   - Replace the "lighter brown with reduced opacity" subtle-text rule: tertiary text uses the walnut AA variants at full opacity. Reduced opacity is for decoration only (fleurons, dividers), never for text a user must read.
   - Add a bullet: on `#D4C4B0` accent panels, text uses `#5C4033` (5.51:1) or darker, because the walnut variants fail there (`#715D45` = 3.67:1).
3. **New sub-section `### Text Contrast (WCAG AA)`.** Insert it between the Special Accents bullets and `### Control Colors`. Include:
   - The rule: 4.5:1 for normal text, 3:1 for large text (≥ 24px, or ≥ 18.66px bold).
   - A pair table with columns: text colour | on background | ratio | use.
     - Rows for the four variants: `#7A654B` on `#F5E6D3` 4.52; `#715D45` on `#EBD9C7` 4.56, noting it is also 5.11 on `#F5E6D3` and so is the walnut that is safe on either paper; `#55703E` on `#F5E6D3` 4.54; `#4E6839` on `#EBD9C7` 4.54.
     - The already-passing text colours for reference: `#3C2F2F` 10.45 and `#5C4033` 7.67 on `#F5E6D3`; button text `#2C3E10` 7.69 on the default tint and 5.02 on the active tint.
     - The originals, marked "borders / fills / decoration only — fails as text": `#8B7355` 3.66, `#6B8E4E` 3.06, `#8BA870` 2.16.
   - A note that each variant is bound to its background: `#7A654B` and `#55703E` fail on `#EBD9C7` (4.03 / 4.05).
   - A note that ratios are WCAG 2.x relative luminance, derived by scaling the original's RGB (R4, UI design review 260924-nho). Text over a background image (paper JPG, botanical plate) is not captured by a colour ratio, so keep reading-critical text on flat colour or check it by eye.
   - The measurement command `node scripts/measure-ui.js --plugin <Name> --contrast`.
4. **Typography → Font Sizing.** Add a bullet beginning with the exact bold phrase `**Text floor: 9px.**`. No rendered text below 9px, including version labels, keyboard note names, units and footers. Text at the floor must still meet 4.5:1.
5. **Example Color Codes.**
   - Change the `--brown-border` comment so it reads walnut borders and decoration, NOT text.
   - Add a `/* Text on paper — WCAG AA 4.5:1 */` group with `--text-walnut: #7A654B;`, `--text-walnut-mid: #715D45;`, `--text-sage: #55703E;` and `--text-sage-mid: #4E6839;`. Each gets a comment naming its background and ratio.
   - Keep every existing variable and value.
6. **Implementation Checklist.** After the "Verify all controls readable and accessible" item, add two items:
   - no text below the 9px floor;
   - text colours taken from the Text Contrast pair table, with `node scripts/measure-ui.js --plugin <Name> --contrast` showing 0 below AA on flat backgrounds.

Do not rename, reorder, add or remove any `## ` header. Do not touch `metadata.json`.

Commit: run `git branch --show-current` and `git status --short` immediately before, then `git commit -m "docs(quick-260924-thi): naturalist template — AA text variants, originals for borders/decoration, 9px floor" -- .claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`.
  </action>
  <verify>
    <automated>SCR="${SCR:?prefix this call with SCR=<your session scratchpad>/thi}"; cd /Users/taylorbrook/Dev/VST-development && F=.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md && grep '^## ' "$F" | diff "$SCR/headers.before" - && grep -qE '^- Warm brown: #8B7355' "$F" && grep -qE -- '--brown-border:' "$F" && test "$(grep -E '^- Warm brown: #8B7355|--brown-border:' "$F" | grep -c 'borders, text')" -eq 0 && grep -E -- '--brown-border:' "$F" | grep -qF 'NOT text' && grep -qF 'Text floor: 9px.' "$F" && grep -qF '### Text Contrast (WCAG AA)' "$F" && grep -qF -- '--text-walnut: #7A654B' "$F" && grep -qF -- '--text-sage-mid: #4E6839' "$F" && test "$(grep -o '#8B7355' "$F" | wc -l)" -ge 16 && test "$(grep -o '#6B8E4E' "$F" | wc -l)" -ge 4 && test "$(grep -o '#8BA870' "$F" | wc -l)" -ge 3 && git diff --quiet -- .claude/aesthetics/ouaricon-naturalist-001/metadata.json && node -e 'const fs=require("fs");const t=fs.readFileSync(process.argv[1],"utf8");const lin=c=>{c/=255;return c<=0.04045?c/12.92:Math.pow((c+0.055)/1.055,2.4)};const L=h=>{const v=[1,3,5].map(i=>parseInt(h.slice(i,i+2),16));return 0.2126*lin(v[0])+0.7152*lin(v[1])+0.0722*lin(v[2])};const R=(a,b)=>{const x=L(a),y=L(b);return(Math.max(x,y)+0.05)/(Math.min(x,y)+0.05)};let bad=0;for(const[f,b]of[["#7A654B","#F5E6D3"],["#715D45","#EBD9C7"],["#715D45","#F5E6D3"],["#55703E","#F5E6D3"],["#4E6839","#EBD9C7"]]){const r=R(f,b);const present=t.includes(f);console.log(f,"on",b,r.toFixed(3),present?"in file":"MISSING");if(r<4.5||!present)bad++}process.exit(bad)' "$F"</automated>
  </verify>
  <done>
- The template carries the four AA variants, each bound to its background and each recomputed at 4.5:1 or above. The originals are kept, at no fewer occurrences than before, and scoped to borders, fills and decoration.
- No line assigns walnut to text.
- The 9px floor, the pair table, the `--text-*` variables and the two checklist items are present.
- The `## ` headers are unchanged and `metadata.json` is untouched.
- The change is committed path-scoped.
  </done>
</task>

<task type="auto">
  <name>Task 3: Baseline --contrast on the four plugins (HEAD snapshot), independent cross-check, README section, SUMMARY table</name>
  <files>scripts/measure-ui-README.md, .planning/quick/260924-thi-r4-contrast-aa-passing-text-variants-9px/260924-thi-SUMMARY.md</files>
  <read_first>
    - scripts/measure-ui-README.md lines 1-38 (usage block, exit table) and 127-276 (four screens, "Reading a 0" controls, known limitations)
    - .planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md lines 586-605 (A.4 table, the comparison point)
  </read_first>
  <action>
**1. Baseline run.**
- For each of O-TextureForge, O-MicrotonalSampler, O-Prism and O-ReverseDelay, run `node scripts/measure-ui.js --plugin <P> --root "$SCR/snap" --contrast --verbose`. Send stdout to `$SCR/base-<P>.json` and stderr to `$SCR/base-<P>.err`, and record each exit code, which must be 0.
- The snapshot is the Task 1 Step 0 HEAD snapshot. Its sha is in `$SCR/head.sha`, and Tasks 1–2 touch nothing under `plugins/`, so the plugin content is exactly that sha.
- Each run takes about 20 s. Run them sequentially in one Bash call, or in the background if the call approaches the 600 s watchdog.

**2. Independent cross-check.** The repo's rule is that a new report's numbers must agree with an independent scan. Write `$SCR/xcheck.js` (scratch, never committed). It takes two arguments, `<rows.json> <stderr file>`, and for that pair it must:
- (a) recompute the WCAG ratio from `ct.fg` and `ct.bg` with its OWN luminance function, and require |Δ| ≤ 0.001 for every counted row;
- (b) recompute per language T, B, pct, F and I from the rows, and require them to equal the numbers parsed from that plugin's per-language stderr lines;
- (c) print `XCHECK OK <P>` or exit 1.

**3. SUMMARY baseline table.** Record, in the executor's SUMMARY.md:
- the snapshot sha;
- per plugin and per language: frame, text nodes, below AA and %, under the 9px floor, over background-image, median/min ratio, and the count of `step skipped` lines under `--verbose` (a coverage disclosure);
- a column with the review's A.4 figure (O-Prism 75%, O-TextureForge 69%, O-MicrotonalSampler 44%, O-ReverseDelay 5%), with one sentence on why the numbers differ:
  - this run walks every state × every language, keyed by DOM path;
  - the review took one default-state pass;
  - O-ReverseDelay's shipping frame is now 940×693.
- For each plugin, the top failing fg→bg pairs grouped with counts (up to 5), each mapped to the template variant that would fix it. This is the worklist for R4's per-plugin half. State plainly that no plugin was edited.

**4. README.** Edit `scripts/measure-ui-README.md`:
- add a `--contrast` example line to the top usage block;
- add a `## The contrast report (opt-in)` section after the four-screens section. It covers what is measured, the thresholds, the output lines, why it is opt-in and outside `all`, and the baseline for the four plugins in `en` at the recorded sha as the reference reading;
- add `**Control 3 — the contrast fixture.**` under "Reading a `0`". Give the fixture recipe from Task 1 Step 5 in prose, as a list of element → expected ratio, plus the exact expected count line and `en:` line. A contrast `0` is only readable beside this control;
- add bullets to "Known limitations, carried openly" matching the header limitations from Task 1.

Commit the README: run `git branch --show-current` and `git status --short` immediately before, then `git commit -m "docs(quick-260924-thi): measure-ui-README — contrast report, Control 3, baseline" -- scripts/measure-ui-README.md`.

Finally, confirm that none of this task's commits touches `plugins/`. SUMMARY.md is committed by the execute workflow's docs step, path-scoped to the quick directory.
  </action>
  <verify>
    <automated>SCR="${SCR:?prefix this call with SCR=<your session scratchpad>/thi}"; cd /Users/taylorbrook/Dev/VST-development && for P in O-TextureForge O-MicrotonalSampler O-Prism O-ReverseDelay; do test -s "$SCR/base-$P.json" && grep -qE '^contrast: [0-9]+ finding\(s\)$' "$SCR/base-$P.err" && grep -qE '^  en: [0-9]+ text node\(s\)' "$SCR/base-$P.err" && grep -qE '^  fr: [0-9]+ text node\(s\)' "$SCR/base-$P.err" && grep -qE '^  zh-Hans: [0-9]+ text node\(s\)' "$SCR/base-$P.err" && node "$SCR/xcheck.js" "$SCR/base-$P.json" "$SCR/base-$P.err" || { echo "FAIL $P"; exit 1; }; done && grep -qF -- '--contrast' scripts/measure-ui-README.md && grep -qF 'Control 3' scripts/measure-ui-README.md && grep -qF '## The contrast report (opt-in)' scripts/measure-ui-README.md && SHAS=$(git log --format=%h --grep='quick-260924-thi' -n 10) && test -n "$SHAS" && for s in $SHAS; do FILES=$(git show --name-only --format= "$s") || exit 1; if printf '%s\n' "$FILES" | grep -q '^plugins/'; then echo "PLUGIN PATH IN $s"; exit 1; fi; done && echo "BASELINE + README + NO-PLUGIN OK"</automated>
  </verify>
  <done>
- All four plugins measured with exit 0 against a recorded-sha snapshot, and each has an `en`, `fr` and `zh-Hans` line.
- The independent cross-check agrees on every ratio and every per-language count.
- SUMMARY.md holds the baseline table beside the review's A.4 figures, plus a per-plugin failing-pair worklist.
- The README documents the report, Control 3 and its limitations, and is committed path-scoped.
- No quick-260924-thi commit touches `plugins/`.
  </done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| repo UI pages → headless Chromium | Plugin HTML/JS (trusted repo content, HEAD snapshot) executes in a local headless page. The state files' `eval` steps already exist and are unchanged |
| saved rows JSON → `--from` | A local file is parsed by JSON.parse and read by pure screen functions. No code runs from it |
| this session → shared checkout | Other sessions' uncommitted work shares the index and HEAD |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-thi-01 | Tampering | git commits in a shared checkout | medium | mitigate | Every commit is `git commit -m … -- <single path>`, with `git branch --show-current` + `git status --short` immediately before. Task 3's verify asserts that no quick-260924-thi commit names a `plugins/` path |
| T-thi-02 | Tampering | measure-ui.js default behaviour (other waves' verify blocks match its output) | medium | mitigate | `ct` is added only when contrast is requested, and `contrast` is kept out of SCREEN_ORDER. Task 1's verify `cmp`/`diff`s stdout+stderr against a pre-edit capture in fonts mode and `--mode box --report all` |
| T-thi-03 | Information disclosure / integrity of numbers | baseline measurement | low | mitigate | Measure a `git archive HEAD` snapshot at a recorded sha, never the working tree (O-ReverseDelay and O-Prism carry other sessions' WIP) |
| T-thi-04 | Repudiation | report correctness (a wrong 0 or % reads as truth) | medium | mitigate | Fixture positive control with 13 hand-computed ratios (Task 1), plus an independent recompute of every ratio and per-language count from the saved rows (Task 3) |
| T-thi-05 | Denial of service | in-page ancestor walk per text node | low | accept | Bounded by DOM depth × visible text nodes (O-Prism ≈ 2.3k keys, run ≈ 19 s). The report only; no CI job invokes it |
| T-thi-06 | Elevation of privilege | `(0, eval)` of state-file steps | low | accept | Pre-existing, unchanged by this plan. The input is tracked repo content |

No package installs: Playwright is resolved from an existing install by `resolvePlaywright()`, which never installs. There is therefore no supply-chain row.
</threat_model>

<verification>
- The Task 1 verify passes: the fixture matches exactly, the default output is byte-identical, `all` is unchanged, the SKIPPED hint is correct, and the exports are intact.
- The Task 2 verify passes: four variants each recomputed at 4.5:1 or above, originals kept, no walnut-as-text line, 9px floor present, headers unchanged, metadata untouched.
- The Task 3 verify passes: four plugins × three languages measured, the cross-check agrees, the README is updated, and no commit touches `plugins/`.
- `git status --short -- plugins/ PLUGINS.md` shows the same foreign entries as before execution. This session added or staged nothing there.
</verification>

<success_criteria>
- The brand template no longer prescribes a failing text colour. The AA variants are bound to their backgrounds, and the originals survive for borders and decoration.
- The 9px floor is written into the template.
- `measure-ui.js --contrast` gives a per-node WCAG ratio and per-language % below AA and under-floor counts. It is report-only (exit 0), and the default output is provably unchanged.
- The baseline for O-TextureForge, O-MicrotonalSampler, O-Prism and O-ReverseDelay is recorded in SUMMARY.md, independently cross-checked, and compared against the review's A.4.
- There are three path-scoped commits, and zero files under `plugins/` are modified.
</success_criteria>

<output>
Create `.planning/quick/260924-thi-r4-contrast-aa-passing-text-variants-9px/260924-thi-SUMMARY.md` containing:
- the baseline table;
- the failing-pair worklist;
- the snapshot sha;
- the determinism note from Task 1 Step 0;
- the planner-derived `#4E6839` addition, with its rationale;
- the three commit shas.
</output>
