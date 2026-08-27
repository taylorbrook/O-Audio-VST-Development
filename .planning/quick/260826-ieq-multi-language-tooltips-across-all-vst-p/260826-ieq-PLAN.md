---
task: 260826-ieq-multi-language-tooltips-across-all-vst-p
type: execute
mode: quick
revision: 2                # re-planned 2026-08-26 after the SCOPE EXPANSION in CONTEXT.md
autonomous: false          # four checkpoints; see the STAGING section
staged: true

stages_complete: [A, B, C, D]
stages_remaining: [E, F, G, H, I, J, K, L, M]

files_modified:
  # ── Stage E, repo-level tooling (no plugin touched) ──
  - scripts/ui-stub/generic-juce-stub.js
  - scripts/serve-ui.js
  - scripts/boot-all-uis.js
  - scripts/i18n-extract.js
  - scripts/check-ui-labels.js
  - scripts/i18n-canon.js                              # canon v2 added alongside v1
  - scripts/check-i18n.js                              # assertions 10-15
  # ── Stages F-L, per plugin, one path-scoped commit each ──
  - plugins/<Name>/<uiroot>/js/i18n.js                 # + LABELS, I18N_EXEMPT
  - plugins/<Name>/<uiroot>/js/app.js                  # canon v2 block (or the inline module)
  - plugins/<Name>/<uiroot>/index.html                 # data-i18n / data-i18n-aria attrs
  - plugins/<Name>/<uiroot>/css/styles.css             # per-plugin overflow fixes ONLY
  - plugins/<Name>/tests/i18n-states.json              # optional, only where a panel is hidden by default
  - plugins/<Name>/Source/Plugin{Processor,Editor}.{h,cpp}
  - plugins/<Name>/CMakeLists.txt
  - plugins/<Name>/CHANGELOG.md
  - PLUGINS.md

estimate:
  tokens: 4500000          # Stages E-L, ALL dispatches summed; NOT a single-agent budget
  raw_tokens: 2250000
  tasks: 12
  confidence: low          # n=1 calibration sample (Stages A-D: 232k actual). See the SIZING section.

must_haves:
  truths:
    - "A French user sees every control label, section heading, button caption and status message in French — not only the tooltips."
    - "Switching language re-renders labels the JS writes as well as labels the HTML authors, with no reload and no stale English survivor anywhere on the page."
    - "Value readouts still show `1.5 kHz` in both languages, and every editable readout still accepts what it displays. (D-03)"
    - "Factory preset names read identically in both languages, and a session saved with the UI in French still recalls its preset. (D-02)"
    - "The host's automation lane and parameter list show the same English parameter names they showed before this work. (D-01)"
    - "No French label clips, ellipsizes, or moves a neighbouring element at any plugin's shipping frame size. (D-04)"
    - "A language choice made in the DAW survives closing and reopening the session — MEASURED, not reasoned."
  artifacts:
    - "scripts/ui-stub/generic-juce-stub.js + scripts/serve-ui.js — render ANY of the 43 pages headless; only 5 have a hand-written stub today"
    - "scripts/i18n-extract.js — the inventory + skeleton generator that makes ~2,400 text nodes tractable"
    - "scripts/check-ui-labels.js — the both-language render gate: coverage, clip, and the en-vs-fr GEOMETRY DIFF that retires D-04"
    - "scripts/i18n-canon.js — canon v2 (the [data-i18n] sweep + setLabel), carried alongside v1 during migration"
    - "plugins/<Name>/<uiroot>/js/i18n.js — LABELS and I18N_EXEMPT added to the existing tooltip table"
  key_links:
    - "data-i18n attribute -> applyI18n's [data-i18n] sweep -> textContent AND dataset.label written together"
    - "setLabel(el, key, vars) -> writes data-i18n + data-i18n-vars -> the SAME sweep owns it from then on (ONE re-render path, no event, no subscription)"
    - "check-ui-labels geometry diff en vs fr -> the named shifted element -> that plugin's own CSS fix"
    - "i18n-canon v2 <-> check-i18n assertion 6 <-> the 43 hand-copies (the only drift mitigation under the no-shared-module rule)"
    - "param-dump TSV (shipped in Stage A) -> generic-juce-stub slider/toggle/combo states -> a page that boots without a hand-written stub"
---

<objective>
**Stages A-D shipped tooltip localization on 5 plugins. This revision replans everything
after, at the expanded scope: ALL user-visible text inside every plugin's WebView UI.**

The five shipped plugins are currently half-localized — French tooltips over English
labels — and their own tooltip copy tells the user that labels do not change, which this
work makes false. They get retrofitted first, on code already trusted, before the pattern
reaches 38 more.

Purpose: a French-speaking user can operate the plugin, not merely read its hover-help.
Output: 43 fully localized WebView UIs, 5 new repo-level tools, and a render gate that
measures French label geometry instead of assuming it.
</objective>

<context>
@/Users/taylorbrook/Dev/VST-development/CLAUDE.md
@/Users/taylorbrook/Dev/VST-development/.planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-CONTEXT.md   # READ THE "SCOPE EXPANSION" SECTION FIRST
@/Users/taylorbrook/Dev/VST-development/.planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-SUMMARY.md   # what Stages A-D actually shipped, incl. "Not verified"
@/Users/taylorbrook/Dev/VST-development/.planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-RESEARCH.md  # still binding on bridge, persistence, resource serving, TDZ ordering, gate mechanics

Shipped reference implementations — read these, not the pre-Stage-A originals:
@plugins/O-Tapestop/Source/ui/public/js/i18n.js               # the table shape, incl. the [selector, key, wrapper] triple
@plugins/O-MultiBandCompressor/Source/ui/public/js/i18n.js    # nested-key {token} vars — the composed-label shape
@plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js       # the committed both-language sweep the label gate generalises
@scripts/i18n-canon.js                                        # canon v1, held between sentinels inside a comment
@scripts/check-i18n.js                                        # discovery + the 9 existing assertions
@scripts/param-dump/README.md                                 # the runtime inventory tool Stage A shipped
</context>

<locked_decisions>
From CONTEXT.md's SCOPE EXPANSION section. Plan HOW, never WHETHER.

- **D-01 — WebView only.** C++ parameter display names in `createParameterLayout()` stay
  English. They are the host automation-lane contract, some hosts cache them, and 43
  plugins are already shipping against them.
- **D-02 — Factory preset names stay English.** The name IS the JSON filename
  (`modules/persistence/preset-manager/cpp/OuariconPresetManager.h:283-285`). No
  indirection layer is added.
- **D-03 — English number formatting retained.** Readouts stay `1.5 kHz`. None of the
  `toFixed()` sites is touched; no editable-readout parser is touched. Unit symbols
  (Hz, dB, ms, %, s) are language-neutral and unchanged.
- **D-04 — Fixed-width overflow is GATED and fixed per plugin.** No auto-shrink font, no
  short-variant fallback. The both-language sweep already committed in three `tests/`
  directories is extended to measure every localized label's rendered box.

Still binding from the pre-expansion decisions: explicit dropdown, English default, no
locale sniffing; per-plugin JS with no shared UI module; EN + FR only; French machine-
drafted and flagged `reviewed: false`; O-ReverseDelay decision D13 (no
`setTooltipsEnabled`, ever) stands.
</locked_decisions>

<facts_measured_during_this_replan>
Verified this session against the tree as it stands AFTER Stage D. Do NOT re-derive.
Several of these correct the numbers in the brief.

**1. The static-text count in the brief is stale and slightly high.**
A scan of all 43 served `index.html` files (comments, `<script>` and `<style>` stripped;
nodes required to contain two consecutive letters) finds **2,438** alphabetic text nodes,
not 2,885. The per-plugin figures also moved, because four of the five Stage A-D plugins
already emptied their markup of tooltip copy:

| Plugin | brief | measured now | | Plugin | brief | measured now |
|---|---|---|---|---|---|---|
| O-Prism | 319 | **247** | | O-Octagon | 128 | **78** |
| O-Lyrica | 227 | **181** | | O-MultiBandCompressor | 123 | **120** |
| O-MicrotonalSampler | 144 | **126** | | O-Bitrot | 102 | **90** |
| O-Formant | 111 | **109** | | O-Polystutter | 105 | **96** |

Light end unchanged in character: O-Bass 12, O-AnalogSaturation 14, O-Chorus 15,
O-DigiDelay 16, O-Texture 16, O-Emulator 17.

**2. A THIRD text source the brief does not name: 386 visible-text ATTRIBUTES.**
`title=` **196**, `aria-label=` **172**, `alt=` 11, `placeholder=` 7. O-Octagon alone
carries 78 `aria-label`s, O-Contrabass 47, O-Orbit 38. A sweep over text nodes and JS
literals misses every one. `aria-label` is the accessible name — it is user-visible text
by any definition that matters.

**3. The "92 JS-written string literals" resolves into three very different populations.**
There are **577** `textContent =` / `innerText =` assignment sites repo-wide. Classified:

| Population | Count | Disposition |
|---|---|---|
| Numeric/unit readouts (`` `${hz.toFixed(1)} Hz` ``, `${Math.round(v*100)}%`) | the large majority of the 577 | **OUT OF SCOPE by D-03.** Not touched. |
| Unique PROSE strings written from script | **53** | In scope. `Free Run`, `Retrig`, `Sample folder not found`, `Browse for folder…`, `No presets available`. |
| COMPOSED templates with prose outside the interpolation | **27** | In scope, hardest shape — needs the `{token}` entries proven on MBC in Stage B. |

Concentration: O-Prism 9 prose, O-Marimba 7, O-MicrotonalSampler 6 prose + 8 composed,
O-Lyrica 5 + 5, O-FreqPulse 2 + 3. Twenty-four plugins have **zero** JS-written prose.
This is a much smaller and much more tractable problem than "92 literals" suggests — but
it is unreachable by any markup sweep, so it still needs its own scanner.

**4. O-MicrotonalSampler inlines English PLURALIZATION.** `` `Issues (${skipped.length}
file${skipped.length === 1 ? '' : 's'} skipped)` ``. French pluralizes zero as singular,
so a direct port is wrong at n=0. Ruling below in the CANONICAL CONTRACT V2 section.

**5. Hard-clip risk is small; WRAP-GROWTH risk is the real one.** Repo-wide there are only
**33** `text-overflow: ellipsis` declarations and ~110 `white-space: nowrap`. Nine plugins
have neither — their labels wrap freely, which means French does not clip them, it makes
them TALLER and pushes their row. That is invisible to a clip check and is exactly what
the geometry diff in `check-ui-labels.js` is built to catch.

**6. Only 5 of 43 plugins have a bridge stub** (`tests/ui-stub/`): O-Bitrot, O-Octagon,
O-ReverseDelay, O-SpectralShaper, O-Tapestop. Only 17 have a `tests/` directory at all.
**Without a generic stub, 38 plugins cannot be rendered, and without rendering, D-04
cannot be discharged on them.** This is the single largest unflagged dependency in the
expansion and it is why Stage E leads.

**7. All 43 plugins expose a parseable `setSize(W, H)` in `Source/PluginEditor.cpp`**, so
the gate can pin each viewport without a per-plugin constant. The tight frames — where
French bites hardest — are O-Chorus 700x125, O-DigiDelay 700x196, O-AnalogEQ 920x220,
O-Bass 420x320, O-SimpleReverb 500x350, O-Gain 350x500. The roomy ones are O-Prism
1200x800 and O-simpleSubtractive 1180x820.

**8. O-Prism has 173 runtime parameters**, confirmed by the Stage A param-dump tool, whose
README records that a regex over its source finds **zero** usable IDs because they are
string-concatenated. It also carries the most static text of any plugin (247). It is its
own stage, batched with nothing.

**9. All five shipped plugins carry tooltip copy that this work makes FALSE.** Their
`lang-select` entries read, verbatim, "the labels on the page itself do not change"
(O-Bitrot `:73`, O-Octagon `:96`, O-ReverseDelay `:87`, O-Tapestop `:78`) and "Control
labels and value readouts stay in English" (O-MultiBandCompressor `:295`). Half of each
sentence stays true under D-03 — readouts DO stay English. The label half must be
rewritten in the retrofit, in both languages, on all five.

**10. Windows/WebView2 verification is blocked on hardware, by a named deferral already in
the repo.** `.github/workflows/ci-tests.yml:140-148` runs `octagon-windows-vst3`, and its
own header comment states it "CANNOT claim that the UI is CORRECT on Windows. No human
sees it this milestone. Named deferral, owner none, blocked on hardware." That job builds
and pluginvals ONE plugin. The i18n-specific Windows risk — WebView2 font metrics differing
from WebKit, so a French label that fits on macOS clips on Windows — cannot be retired
without hardware. It is carried as a named deferral, not planned away.
</facts_measured_during_this_replan>

<canonical_contract_v2>
Canon v1 (Stages A-D) localizes tooltip ATTRIBUTES. Canon v2 adds labels, attributes and
JS-written strings. `scripts/i18n-canon.js` carries BOTH; `check-i18n.js` accepts either
and reports the split as a migration worklist, so the repo is never red mid-migration.

### 1. Labels are owned by the ELEMENT, via `data-i18n`

```html
<span class="knob-label" data-i18n="knob.depth">Depth</span>
<button id="mode-free" data-i18n="mode.free" data-i18n-aria="mode.free.aria">Free</button>
<input id="preset-filter" data-i18n-placeholder="preset.filter">
```

The authored English stays in the markup as the fallback that renders if `applyI18n` never
runs. A key on the element beats a selector in `i18n.js`: `document.querySelector` returns
the FIRST match in document order, which is precisely how O-Octagon's `.vunit-group` tip
nearly landed on the wrong control in Stage C.

### 2. `applyI18n` gains a label sweep that writes `textContent` AND `dataset.label` together

```js
function applyLabel(el) {
    const key = el.dataset.i18n;
    if (!key) return;
    let vars = null;
    try { vars = el.dataset.i18nVars ? JSON.parse(el.dataset.i18nVars) : null; }
    catch (e) { console.warn(`i18n: bad vars on ${key}`); }
    const s = tr(key, uiLanguage, vars);
    el.dataset.label = s.t;      // the ownership mirror — read by state updaters
    el.textContent   = s.t;
}
```

**Why both.** `pattern_js_state_updater_overwrites_html_labels` is the failure mode that
erased authored captions once already, and making every label JS-written puts all 43
plugins into it at the same time. The repo's documented fix is that the element OWNS its
label and the updater reads `el.dataset.label ?? fallback` instead of a hard-coded string.
Writing both in one place makes the invariant checkable at render time:
`el.dataset.label === el.textContent` for every `[data-i18n]`, asserted after page init,
after a language switch, and after a state-update pass.

### 3. A JS-written label declares its own key — ONE re-render path

```js
export function setLabel(el, key, vars) {  // lives in the canon block, not in i18n.js
    if (!el) return;
    el.dataset.i18n = key;
    if (vars) el.dataset.i18nVars = JSON.stringify(vars); else delete el.dataset.i18nVars;
    applyLabel(el);
}
```

`el.textContent = 'Free Run'` becomes `setLabel(el, 'mode.freeRun')`. The element is a
`[data-i18n]` element from that moment on, so the language-change sweep owns it. **No
custom event, no subscription list, no second code path that can go stale in the other
language.** That is the whole point: a state-dependent string written outside the table is
stranded in the previous language the instant the selector fires — the exact bug Stage B
found on MBC's hover-help toggle, generalised.

### 4. Attributes

`data-i18n-aria` and `data-i18n-placeholder` resolve through the same sweep with
`setAttribute`. **Native `title=` is DELETED**, not localized: on an element that has a
`data-tip` it produces a second, untranslated OS tooltip competing with the measure-then-pin
renderer. Where a `title=` is the only help an element has, its text moves to
`data-i18n-aria`. Inventing new tooltip prose is Stage M's job, not this rule's.

### 5. Number and unit rule (D-03)

A readout is never a `[data-i18n]` element. `` el.textContent = `${hz.toFixed(1)} Hz` ``
is untouched. Where a readout and a label share a node, they are SPLIT into two spans and
only the label span gets a key. Splitting a node is a markup change that can move geometry
— the label gate's diff catches it, which is why the split is safe to make mechanically.

### 6. Plural forms are AVOIDED, not engineered

French pluralizes zero as singular; English does not. Rather than build a plural engine
for one plugin, copy that needs a count is authored to sidestep the inflection —
`Issues (3 files skipped)` becomes a form that reads correctly at 0, 1 and n in both
languages, decided per string when it is authored. `check-i18n` assertion 13 rejects a
ternary inside a `setLabel` call so this cannot creep back.

### 7. `I18N_EXEMPT` — reasoned exclusions, never silence

```js
export const I18N_EXEMPT = [
    ['Cathedral', 'factory preset name — the name IS the filename (OuariconPresetManager.h:283)'],
    ['Hz',        'unit symbol, language-neutral (D-03)'],
    ['Français',  'endonym — a language name is never translated'],
];
```

Every visible string the coverage scan finds must be either a `[data-i18n]` element, a
`setLabel` call, or an `I18N_EXEMPT` entry **with a reason**. A bare skip list would let a
missed label hide as a deliberate one; O-Octagon's reviewed-when-it-grows whitelist is the
precedent for reason-bearing exclusions.

### 8. Migration is explicit, so the repo is never red

`i18n-canon.js` exports `I18N_CANON_V1` and `I18N_CANON_V2`. `check-i18n.js` assertion 6
passes on either and prints, per plugin, which canon it is on plus a repo total. A
`--strict-v2` flag fails anything still on v1; it is not the default until Stage L lands.
The alternative — changing the canon in place — turns `check-i18n.js` red the moment
Stage E commits and keeps it red for the whole rollout, which normalises a red gate.
</canonical_contract_v2>

<staging>
Nine stages. Every stage is independently shippable because every plugin commit is
path-scoped to one plugin. Dispatch boundaries are marked; Stage E is two dispatches.

| Stage | Task | Scope | Size (tokens) | Confidence |
|---|---|---|---|---|
| ~~A~~ | ~~T1, T2~~ | ~~param-dump, i18n-canon, check-i18n~~ | **DONE** `5ee01ed4`, `cceebfa4` | |
| ~~B~~ | ~~T3~~ | ~~O-MultiBandCompressor v1.10.0~~ | **DONE** `b98aa9ec` | |
| ~~C~~ | ~~T4~~ | ~~O-Octagon v1.6.0~~ | **DONE** `8dcb1317` | |
| ~~D~~ | ~~T5~~ | ~~O-ReverseDelay / O-Bitrot / O-Tapestop~~ | **DONE** `951dd584` `1f3a9faa` `547f9738` | |
| **E** | T6, T7 | Rendering: generic stub + serve + boot-all + extractor | 150k-250k | low |
| **E** | T8, T9 | Gates: canon v2, check-i18n 10-15, check-ui-labels | 120k-200k | low |
| **F** | T10 | **Retrofit pattern-bearer: O-Tapestop** | 120k-180k | low |
| **G** | T11 | **Retrofit gate-hardening: O-Octagon** | 130k-200k | low |
| **H** | T12 | Retrofit MBC, O-Bitrot, O-ReverseDelay | 300k-450k | low |
| **I** | T13 | The remaining 9 `data-tip` plugins | 700k-1.1M | very low |
| **J** | T14 | Port + localize the 7 second-renderer plugins | 700k-1.0M | very low |
| **K** | T15 | The 21 remaining bare plugins — LABELS ONLY | 1.2M-1.9M | very low |
| **L** | T16 | **O-Prism, alone** | 200k-350k | very low |
| **M** | T17 | Author English hover-help for the bare plugins | 800k-1.5M | **RECOMMEND DEFER** |

### Why Stage E leads, and why it is not optional

Fact 6: 38 of 43 plugins cannot be rendered headless today. D-04 says overflow is GATED.
A gate that cannot load the page is not a gate. Everything downstream of E is hand-work
without it and review-work with it — Stage E is the difference between reading 2,438 text
nodes and reviewing a generated worklist.

It also produces the first honest answer to a question nobody has asked: **do all 43 pages
even boot headless?** `boot-all-uis.js` will answer that in one run, and any page that
throws is a finding worth having before it is a blocker.

### Why O-Tapestop is the label pattern-bearer, not O-MultiBandCompressor

MBC was the right choice for tooltips because its `applyTooltip()` was already the
key-indirection design. For labels the criteria are different:

1. **860 x 580 is a tight frame.** French label overflow is a geometry problem, and
   proving the gate on a roomy frame proves nothing. O-Tapestop is the second-tightest of
   the five shipped plugins and the tightest with an existing stub.
2. **52 text nodes** — small enough that one agent does all of them well rather than
   mechanically. MBC has 120 and O-Prism 247.
3. **It already has both a `tests/ui-stub/` and a committed both-language clamp gate**, so
   the new repo-level label gate can be cross-checked against a plugin whose rendering is
   already known-good. If `check-ui-labels.js` disagrees with `ui_tooltip_clamp_check.js`
   about O-Tapestop's geometry, the tool is wrong, not the plugin.
4. **Zero JS-written prose strings.** Stage F proves the label mechanism without also
   proving `setLabel`. Stage H's MBC does the opposite. Decoupling the two proofs is the
   same reasoning that put MBC before O-Octagon in the original plan.
5. Its Stage D commit is the most recent of the five, so its shape is the freshest copy of
   the settled contract.

### Safe stop points, in order of attractiveness

- **End of Stage H — the recommended partial ship.** Five plugins FULLY localized rather
  than half-localized, the D-04 geometry claim measured on five real frames including two
  tight ones, the DAW persistence round-trip finally executed by hand, and the tooling in
  place so the remaining 38 are review-work. The five stop lying to the user about what
  changes when they pick Français.
- **End of Stage E.** Tools only, no plugin touched, nothing regresses. A clean stop if
  the boot-all report turns up something that changes the plan.
- **End of Stage J.** Every plugin that has ever had tooltip copy is fully localized, and
  there is ONE tooltip renderer repo-wide. The 22 bare plugins keep their status quo,
  which is not a regression — they have no hover-help today either.
- **End of Stage L.** All 43 localized for labels. `check-i18n --strict-v2` can become the
  default here and canon v1 can be deleted.
- **Anywhere inside I, J, K.** Each plugin is one path-scoped commit.

**Do NOT stop between Stage E and Stage F.** Stage E leaves canon v2 in the tree with
zero plugins on it. That is harmless but it is also worthless, and the five shipped
plugins keep telling the user their labels will not change.

### The Stage M question — flagged, with a recommendation

The original Stage G bundled two unrelated jobs: authoring English hover-help prose for
22 plugins that have none, and localizing text. The expansion separates them cleanly.

**Recommendation: split them. Do the labels (Stage K/L), defer the authoring (Stage M) to
its own task.** Three reasons:

1. **The volume grew, not shrank.** The original estimate was ~520 parameters. The Stage A
   param-dump already proved that estimate was a regex undercount on the two plugins it
   checked — O-Prism alone is 173, not 81. The real figure is plausibly 700-900.
2. **It is content work with a different acceptance test.** Every other stage is verified
   by a gate. "Is this sentence accurate about what this knob does?" is verified by a human
   who knows the DSP, one plugin at a time. Bundling it with engineering makes both worse.
3. **Nothing depends on it.** Labels ship without it. It cannot regress anything, because
   these plugins have no tooltip copy today.

Against deferring: a French user of O-Bass gets French labels and no hover-help at all,
which is the same help they get in English today. That is a consistency argument, not a
regression argument.

**This is a decision for Checkpoint 6, not a decision this plan makes.** Stage M stays in
the table, unstarted, with an honest size.
</staging>

<sizing>
The previous plan estimated 900k for 8 tasks and the brief records that it "proved low on
parameter volume". Being honest about why, rather than confident and wrong again:

**The one calibration sample.** `260826-ieq-SUMMARY.md` frontmatter records
`actuals.tokens: 232000` for the Stage D dispatch — three plugins, tooltip-only, on a
pattern already proven twice, including three gate rewrites and eleven negative controls.
That is roughly **75k per plugin** for the best case in this task's history.

**Why the expansion costs more per plugin, quantified:**

| Cost driver | Tooltip stage | Label stage |
|---|---|---|
| Strings per plugin | 20-55 tooltips | 12-247 text nodes + attributes + JS prose |
| Markup edits | copy REMOVED from HTML | a `data-i18n` attribute ADDED to every label |
| Gate work | rewrite an existing gate | run a new gate, then FIX what it finds |
| Failure mode | a tip renders wrong | the layout moves, needing a CSS judgement call |

The last row is the one that resists estimation. A geometry diff that names eleven shifted
elements is eleven separate design decisions inside that plugin's visual system, and there
is no way to know the count before the gate runs. **The per-plugin ranges above are 1.5x to
2.5x the tooltip cost, and the upper bound is the honest one for any plugin with a tight
frame.**

**What would make these numbers wrong, in the direction of worse:**

- The generic stub fails on a class of page nobody anticipated (a page that hard-depends on
  a preset-list JSON shape, say). Stage E grows; the affected plugins need hand-written
  stubs after all.
- The geometry diff is noisy — sub-pixel text metrics differing between runs would drown
  the signal. Mitigated by a 0.5px tolerance and by cross-checking against O-Tapestop's
  known-good clamp gate in Stage F, but not yet proven.
- A plugin's layout turns out to need a real redesign in French rather than a CSS tweak.
  D-04 forbids auto-shrink and short variants, so the remaining move is to change the
  layout — which is a design conversation, not a task step.

**Confidence is `low` because n=1**, and `very low` on Stages I-M because no plugin in
those stages has ever been rendered headless. It is derived from the sample count, not
self-rated.
</sizing>

<tasks>

<task type="tracer" tdd="false">
  <name>T6 (Stage E, dispatch 1): Render ANY plugin page headless — generic stub + server</name>
  <files>scripts/ui-stub/generic-juce-stub.js, scripts/serve-ui.js, scripts/boot-all-uis.js, scripts/ui-stub/README.md</files>
  <read_first>plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js:170-195 (buildRoot + serve — the tree-assembly and port-0 pattern to factor out), plugins/O-Octagon/tests/ui-stub/juce-stub.js (a hand-written stub, to learn the surface a generic one must cover), scripts/param-dump/README.md (the TSV columns that seed slider/toggle/combo states)</read_first>
  <action>
This is the tracer: one thin path proving a page can be rendered for a plugin that has
never had a stub, end to end, before anything is built on top of it. Nothing downstream
works without it (fact 6).

`scripts/serve-ui.js` exports `buildRoot(pluginName)` and `serve(root)`, factored from the
three committed clamp gates rather than reinvented. It resolves the SERVED UI root by
probing `Resources/ui` before `Source/ui/public` — the order matters, because
O-MicrotonalSampler and O-Orbit both carry a `Source/ui/public` that is a build-time
staging directory and is not embedded. It copies the production tree byte-identical,
overlays the bridge stub at `js/juce/index.js`, and overlays any resource CMake embeds
from outside the UI root (the vendored preset-manager copy is the known case). It listens
on port 0 and returns the assigned port: a fixed port silently serves another concurrent
session's files, which has already happened in this repo.

`scripts/ui-stub/generic-juce-stub.js` covers the JUCE 8 WebView surface: the `Juce`
ES-module namespace export (NOT `window.__JUCE__` — a stub that only populates the global
leaves every `import` unresolved), `getNativeFunction`, `getSliderState`,
`getToggleState`, `getComboBoxState`, and `backend.addEventListener` / `emitEvent`.
Unknown native-function names resolve a benign default rather than rejecting — the
opposite of the hand-written stubs, deliberately, because their rejection behaviour exists
to serve a set-equality census this tool is not running. A small override map supplies the
handful of names whose return SHAPE matters, `getUiLanguage` returning `en` among them.
Parameter states are seeded from the plugin's param-dump TSV where one exists and from
neutral defaults where it does not.

**A per-plugin `tests/ui-stub/juce-stub.js` always wins when present.** The five that have
one keep using it, so this tool cannot regress a gate that already passes.

`scripts/boot-all-uis.js` loads all 43 pages in Chromium at each plugin's own
`setSize(W, H)` parsed from `Source/PluginEditor.cpp`, and reports per plugin: page
errors, `console.error` calls, unresolved module imports, and the count of elements
carrying text. **Report the failures, do not fix them here** — a page that will not boot
is a finding that may change the plan, and it belongs in the Checkpoint 3 report rather
than in a tool commit.

Exit 77 when Playwright is unresolvable, matching the repo convention, so "nothing was
verified" can never read as a pass.
  </action>
  <verify>
    <automated>node scripts/boot-all-uis.js 2>&amp;1 | tail -20; echo "exit=$?"</automated>
  </verify>
  <done>
`node scripts/boot-all-uis.js` renders every one of the 43 pages and prints a per-plugin
verdict. O-Tapestop, O-Bitrot, O-ReverseDelay, O-Octagon and O-SpectralShaper boot through
their OWN committed stubs, unchanged. At least 35 of the remaining 38 boot clean through
the generic stub; any that do not are named with the specific error. Re-running
`node plugins/O-Tapestop/tests/ui_tooltip_clamp_check.js` still exits 0 — the factoring
did not disturb a committed gate.
  </done>
  <precondition>Playwright resolves in this checkout; the three committed clamp gates exit 0 today, which proves it.</precondition>
</task>

<task type="auto" tdd="false">
  <name>T7 (Stage E, dispatch 1): The inventory + skeleton generator</name>
  <files>scripts/i18n-extract.js, scripts/i18n-extract-README.md</files>
  <action>
Makes ~2,438 text nodes plus 386 attributes plus 80 JS strings tractable. Without it every
plugin stage is transcription; with it every plugin stage is review.

`node scripts/i18n-extract.js --plugin <Name>` emits `plugins/<Name>/.planning/i18n-inventory.tsv`
with one row per candidate string, and a column naming which of the three scanners found
it, because they have different reliability:

1. **HTML text nodes.** Parse the served `index.html`, strip comments, `<script>` and
   `<style>`, and walk visible text nodes. Record the node's enclosing element, its id or
   generated path, and whether that element is already `[data-i18n]`.
2. **Visible attributes.** `title`, `aria-label`, `placeholder`, `alt`. Fact 2 is why this
   scanner exists at all; it is the one a markup-text sweep and a JS sweep both miss.
3. **JS-written strings.** Scan the controller module — which is `js/app.js` on most
   plugins and an inline `<script type="module">` on O-Bitrot, so it must be read from
   wherever it lives, the same wrong-shaped assumption `check-i18n.js` already had to fix
   in Stage D. Collect quoted `textContent`/`innerText` assignments, template literals
   with prose OUTSIDE the interpolations, and `setAttribute` calls on the four visible
   attributes.

Classify each row: `LABEL` (localize), `READOUT` (numeric or unit-only — exempt under
D-03), `ENDONYM`, `PRESET-NAME` (exempt under D-02), `UNIT`, `UNSURE`. Classification is a
**suggestion with a reason**, never a silent drop: an `UNSURE` row is a row a human reads.
The rule for `READOUT` is that the string, with interpolations removed, contains no run of
two letters that is not a known unit token.

Then emit two draft artefacts the plugin task edits rather than authors:
- a `data-i18n` attribute patch against `index.html`, with a key derived from the element's
  id or its section plus its text, uniqueness-checked across the plugin — O-Tapestop's
  Stage D generator caught a real key collision this way before it shipped;
- a skeleton `LABELS` block for `i18n.js` with the English filled in **verbatim from the
  source** and the French left as a `TODO` marker that `check-i18n` assertion 5 already
  rejects, so an unfilled skeleton cannot pass.

`--all` runs every plugin and prints a size-ranked table — the worklist Stages I-L consume.
  </action>
  <verify>
    <automated>node scripts/i18n-extract.js --plugin O-Tapestop &amp;&amp; wc -l plugins/O-Tapestop/.planning/i18n-inventory.tsv &amp;&amp; node scripts/i18n-extract.js --plugin O-Bitrot &amp;&amp; node scripts/i18n-extract.js --all | tail -50</automated>
  </verify>
  <done>
O-Tapestop's inventory lists ~52 HTML rows plus its attributes, with zero JS-prose rows
(fact 3). O-Bitrot's inventory is non-empty for JS rows despite having no `js/app.js` —
the inline-module path works. `--all` covers 43 plugins and its `LABEL` totals land within
10% of the measured 2,438 + 386. Spot-checking twenty `READOUT` classifications by hand
finds zero real labels misfiled as readouts; misfiling in the other direction is
acceptable, because an over-reported candidate costs a human one glance and an
under-reported one ships English.
  </done>
</task>

<task type="auto" tdd="false">
  <name>T8 (Stage E, dispatch 2): Canon v2 + check-i18n assertions 10-15</name>
  <files>scripts/i18n-canon.js, scripts/check-i18n.js</files>
  <read_first>scripts/i18n-canon.js in full (the sentinel-in-a-comment mechanism, so v2 is added the same way rather than a second mechanism), scripts/check-i18n.js:269-310 (discovery) and :440-475 (assertions 6 and 8)</read_first>
  <action>
Add `I18N_CANON_V2` to `i18n-canon.js` between a second sentinel pair, using the existing
read-my-own-source mechanism — a template literal would require escaping the backticks in
`tr()` and an escaped canon is no longer a verbatim canon. V2 is v1 plus `applyLabel`, the
`[data-i18n]` sweep inside `applyI18n`, `setLabel`, and the attribute resolution, exactly
as specified in the CANONICAL CONTRACT V2 section.

Assertion 6 accepts **either** canon and records which. Print a per-plugin canon version
and a repo split line. Add `--strict-v2` to fail v1 plugins; it is not the default until
Stage L. Section 8 of the CANONICAL CONTRACT V2 section is the reasoning — a gate that goes red on
commit and stays red for a whole rollout teaches the team to ignore it.

New assertions, in the existing `check(cond, msg)` style with the exit code as the failure
count:

10. **Label coverage.** Every HTML text node the extractor classifies `LABEL` sits inside a
    `[data-i18n]` element, or matches an `I18N_EXEMPT` entry. **Without this a plugin
    passes at 100% tooltip coverage with every label still hard-coded English** — the
    assertion the whole expansion is unverifiable without.
11. **Attribute coverage.** Every `aria-label` / `placeholder` / `alt` is keyed, or exempt.
    **Zero native `title=` attributes remain** in the served markup (contract section 4).
12. **JS-string coverage.** Every prose string written to `textContent` / `innerText` in
    the controller module goes through `setLabel`, or is exempt. Composed templates with
    prose outside their interpolations are flagged individually — those need `{token}`
    entries, not flat ones.
13. **No inflection logic inside a localized string.** A ternary or a conditional plural
    suffix inside a `setLabel` argument fails (contract section 6, fact 4).
14. **Every `I18N_EXEMPT` entry carries a non-empty reason string.** A bare skip list hides
    a missed label as a deliberate one.
15. **`LABELS` keys resolve.** Every `data-i18n` value in the markup exists in `I18N`, and
    every `I18N` label key is referenced by at least one element or `setLabel` call — a
    dead key is a translation nobody sees and drifts silently.

Assertion 3 currently requires the markup to be empty of tooltip-copy literals. It must not
start rejecting `data-i18n`, which is a KEY and not copy. Check that explicitly; a
substring match on `data-tip` would catch `data-i18n` on neither spelling today but the
next attribute added could collide.

**Run all six negative controls.** A gate that has never been seen to fail proves nothing —
delete one `data-i18n` from a fixture, add a bare `title=`, write a raw literal to
`textContent`, put a ternary in a `setLabel`, empty one exempt reason, orphan one key. Each
must fail with its own assertion named. Apply each to a byte-exact backup and restore FROM
THAT BACKUP; `git checkout --` would wipe the uncommitted tool alongside the mutation.
  </action>
  <verify>
    <automated>node scripts/check-i18n.js; echo "exit=$?"; node scripts/check-i18n.js --strict-v2; echo "strict-exit=$?"</automated>
  </verify>
  <done>
`node scripts/check-i18n.js` exits **0** against the tree as it stands — five plugins, all
on canon v1, all reported as v1 in the split line. `--strict-v2` exits **5**, naming each.
All six negative controls fail with the specific assertion. The 200 unreviewed-French
worklist still prints.
  </done>
  <reversibility rating="costly">Canon v2 sets the label idiom that 43 hand-copies inherit under a rule that forbids fixing it centrally later. Changing the `data-i18n` attribute name or the `dataset.label` mirror after Stage H costs 43 edits.</reversibility>
</task>

<task type="auto" tdd="false">
  <name>T9 (Stage E, dispatch 2): check-ui-labels.js — the both-language render gate</name>
  <files>scripts/check-ui-labels.js, scripts/check-ui-labels-README.md</files>
  <read_first>plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js:340-470 (the language-parameterised sweep, the vacuity guards, and the per-anchor assertion shape this generalises)</read_first>
  <action>
The deliverable that retires D-04. It generalises the committed both-language sweep rather
than starting fresh — same shape, same vacuity guards, same exit-77 convention, same
per-language failure labelling, because an unlabelled French-only failure reads as a
mysterious regression in a file that never mentions French.

**It is repo-level, one file, invoked per plugin** — `node scripts/check-ui-labels.js
--plugin <Name>`. This is a deliberate departure from the three per-plugin clamp gates.
The CONTEXT no-shared-module rule governs shipped UI runtime code, not test tooling;
`scripts/check-i18n.js` is already the precedent for a repo-level per-plugin gate; and 43
hand-copies of a 700-line Playwright file is precisely the drift this repo has paid for
twice. Per-plugin knowledge lives in an optional `plugins/<Name>/tests/i18n-states.json`.
**Flag this at Checkpoint 3** — it is the one structural decision in Stage E that the
developer may want to overrule.

Viewport comes from `setSize(W, H)` in that plugin's `PluginEditor.cpp` (fact 7). Never a
mirrored literal: a fixture constant that mirrors a plugin constant has drifted silently in
this repo twice.

For `en` then `fr`, driven by `window.__setLanguage`, snapshot `getBoundingClientRect()`
for every `[data-i18n]`, every element with an id, and every `[data-i18n]`'s parent. Then:

1. Every `[data-i18n]` has non-empty text and no surviving `{token}` placeholder.
2. **Vacuity guard — French actually rendered.** A meaningful fraction of labels must DIFFER
   between the two passes. Without it a run where `__setLanguage` silently did nothing
   measures English twice and reports a confident, worthless pass. Stage D proved this
   catches a real no-op on all three plugins it was tried against.
3. **`dataset.label === textContent`** for every `[data-i18n]`, asserted after page init,
   after the language switch, AND after a state-update pass. This is the systemic form of
   `pattern_js_state_updater_overwrites_html_labels`.
4. **No clip.** For each `[data-i18n]` whose computed `overflow` is not `visible`,
   `scrollWidth <= clientWidth + 1`. Applied only to leaf label elements — `scrollHeight`
   does not cross a `flex: 1` stage and is clamped on a grid container, so it lies about
   any container, and a gate built on it would certify a real overflow.
5. **No spill.** Each `[data-i18n]` rect stays inside its offsetParent's padding box.
6. **Nothing crosses the frame.** No rect exceeds W x H, and the document's own scroll
   extent does not exceed it either, in either language.
7. **THE GEOMETRY DIFF — the primary detector.** For every element that is NOT a
   `[data-i18n]` and not a descendant of one, its rect must be identical within 0.5px
   between `en` and `fr`. The frame is fixed, so any non-label element that MOVED was
   pushed by a French string. Report every one by id or selector path with its delta.
   This uses `getBoundingClientRect` only, so it is immune to the `scrollHeight` traps in
   assertion 4, which is why it and not the clip check is the load-bearing one — nine
   plugins have no `ellipsis` and no `nowrap` at all (fact 5), so French makes their labels
   TALLER rather than clipped, and only the diff sees that.
8. **No new overlap.** Two `[data-i18n]` rects disjoint in `en` must not intersect in `fr`.

**Hidden state.** `plugins/<Name>/tests/i18n-states.json` optionally lists extra states to
drive — `[{ "name": "free mode", "click": "#mode-free" }]` — re-running the whole sweep in
each. When it is absent the gate REPORTS how many `[data-i18n]` elements were never visible,
so a coverage hole is visible rather than silent. O-ReverseDelay's sync/free swap is the
known case; the settings popover is another, and Stage D already learned that the popover
must be swept OPEN because its two controls sit in the corner where geometry bites hardest.

**Two negative controls before this is trusted:** stub `__setLanguage` to a no-op and
confirm assertion 2 fires; widen one French label by 200px in a fixture and confirm
assertion 7 names the neighbour that moved.
  </action>
  <verify>
    <automated>node scripts/check-ui-labels.js --plugin O-Tapestop; echo "exit=$?"; node plugins/O-Tapestop/tests/ui_tooltip_clamp_check.js; echo "clamp-exit=$?"</automated>
  </verify>
  <done>
Against O-Tapestop before its retrofit the gate runs to completion and reports **zero
`[data-i18n]` elements found**, exiting non-zero with "nothing to measure" rather than
exiting 0 — a gate that passes because there is nothing to check is the vacuous pass this
whole design is built to prevent. Both negative controls fire. O-Tapestop's own committed
clamp gate still exits 0, and its geometry numbers agree with this tool's for the elements
both measure. Neither gate exits 77.
  </done>
</task>

<task type="checkpoint:decision">
  <name>Checkpoint 3: The boot report, and the one structural decision in Stage E</name>
  <action>
Stop. Four tools are in the tree and no plugin has been touched. Report:

**The boot verdict.** How many of the 43 pages render headless through the generic stub,
and the specific failure for each that does not. This is the first time these pages have
been rendered outside their own plugin, and a class of failure here changes Stages I-L.

**The extractor totals.** The per-plugin `LABEL` counts against the measured 2,438 + 386 +
80. Where the extractor disagrees with the hand measurement in the MEASURED FACTS section,
say which is right and why.

**The decision to put to the developer: `check-ui-labels.js` is repo-level, not
per-plugin.** The three tooltip clamp gates live in `plugins/<Name>/tests/`. This one does
not. The reasoning is in T9 — no-shared-module governs shipped UI code rather than test
tooling, `check-i18n.js` is the precedent, and 43 copies of a Playwright gate will diverge.
The cost is that the label gate does not run from a plugin's own `tests/` directory the way
every other gate in that directory does, so a plugin-scoped CI job would need to know about
it. Present both and let the developer choose; this is cheap to reverse now and expensive
after Stage H.

Also confirm: the `data-i18n` attribute name, the `dataset.label` mirror, and `setLabel`
as the single re-render path. That shape reaches 43 plugins under a rule that forbids
fixing it centrally afterwards.
  </action>
  <done>Developer has seen the boot report and either approved the repo-level gate or asked for a per-plugin one, and has approved or amended the canon v2 label idiom.</done>
</task>

<task type="auto" tdd="false">
  <name>T10 (Stage F): Retrofit pattern-bearer — O-Tapestop</name>
  <files>plugins/O-Tapestop/Source/ui/public/js/{i18n.js,app.js}, plugins/O-Tapestop/Source/ui/public/index.html, plugins/O-Tapestop/Source/ui/public/css/styles.css, plugins/O-Tapestop/tests/i18n-states.json, plugins/O-Tapestop/CMakeLists.txt, plugins/O-Tapestop/CHANGELOG.md</files>
  <read_first>plugins/O-Tapestop/Source/ui/public/js/i18n.js:20-60 (the header comment recording every binding constraint, which the retrofit must extend rather than replace), plugins/O-Tapestop/tests/ui_tooltip_clamp_check.js (the gate that must stay green)</read_first>
  <action>
Take one already-trusted plugin from half-localized to fully localized, and prove canon v2
end to end on the tightest shipped frame that has a stub.

1. **Generate the inventory.** `node scripts/i18n-extract.js --plugin O-Tapestop`. Review
   every `UNSURE` and every `READOUT` row by hand. A readout misfiled as a label ships a
   translated number and violates D-03; a label misfiled as a readout ships English.
2. **Fix the two false sentences first.** The `lang-select` entry at `js/i18n.js:78-80`
   promises, in both languages, that the labels on the page do not change (fact 9). Rewrite
   both so they say what is now true: the hover help AND the labels change; the value
   readouts stay in English, per D-03. Do this before anything else so the retrofit cannot
   ship a plugin that contradicts itself.
3. **Add `LABELS` entries to `I18N`** and `data-i18n` attributes to `index.html`, from the
   generated patch. English is MOVED verbatim from the markup, never re-typed — Stage D
   compared its generated table back against the source byte-for-byte on all three plugins
   and that check is what proved nothing was hand-transcribed. Do the same here.
4. **Attributes.** O-Tapestop carries 15. Key the `aria-label`s; delete any native `title=`
   per contract section 4.
5. **Upgrade the canon block to v2** in `app.js`, verbatim from `scripts/i18n-canon.js`.
   The `applyI18n` call site does not move — it stays inside the existing deferred init in
   its own try/catch, because a translation-table typo taking the rest of initialisation
   down is the v1.4.0 TDZ failure this repo has already paid for once.
6. **Run the label gate and FIX WHAT IT FINDS.** `node scripts/check-ui-labels.js --plugin
   O-Tapestop`. Every element the geometry diff names is a per-plugin CSS decision inside
   O-Tapestop's own visual system. **D-04 forbids auto-shrink and short-variant fallbacks**
   — the moves available are widening a container that has slack, letting a label wrap where
   the row can afford the height, or adjusting a grid track. Record each fix and its reason
   in the CHANGELOG.
   **The 860 x 580 frame is a Locked Decision from a prior plan — nothing may move it.**
7. **Cross-check the new tool against the old gate.** `ui_tooltip_clamp_check.js` must
   still exit 0 with the same anchor counts and the same clamp/flip numbers Stage D
   recorded (35 anchors, 12 clamped per language, 4 flipped). A change there means the
   retrofit moved geometry the label gate did not report, which is a bug in the TOOL.
8. Add `tests/i18n-states.json` for the sync/free swap slots and the settings popover if the
   gate reports unseen elements.
9. Version bump, CHANGELOG, PLUGINS.md row, then `./scripts/build-and-install.sh O-Tapestop`
   — which performs the AU-cache clear and the `-dev` / unsuffixed dual-variant sweep. The
   script does not rebuild the Standalone app, so do not judge the UI there.
10. **Stage `i18n.js` and any new file EXPLICITLY before committing.** `git commit -- <path>`
    does not stage untracked files and silently dropped the single most important file in
    the O-ReverseDelay commit in Stage D. Re-check `git branch --show-current` and
    `git status --short` IMMEDIATELY before committing — another session shares this index —
    then `git commit -- plugins/O-Tapestop PLUGINS.md`, and hold back any PLUGINS.md row
    belonging to another session. Never `git add -A`, never `git commit -a`. Verify with
    `git show --stat` afterwards.
  </action>
  <verify>
    <automated>node scripts/check-i18n.js --plugin O-Tapestop &amp;&amp; node scripts/check-ui-labels.js --plugin O-Tapestop &amp;&amp; node plugins/O-Tapestop/tests/ui_tooltip_clamp_check.js &amp;&amp; ./scripts/build-and-install.sh O-Tapestop</automated>
    <human-check>In the DAW: open the gear, pick Français. Every knob label, section heading and button caption turns French — not only the tooltips. Every value readout still reads like `250 ms`, with a period and an English unit. Every factory preset name still reads in English and still loads.</human-check>
  </verify>
  <done>
`check-ui-labels.js --plugin O-Tapestop` exits 0 with the geometry diff reporting **zero**
non-label elements moved between English and French, the vacuity guard confirming French
actually rendered, and `dataset.label === textContent` holding after init, after the switch
and after a state pass. `check-i18n.js --plugin O-Tapestop` exits 0 and reports the plugin
on **canon v2**. The clamp gate still exits 0 — not 77 — with Stage D's anchor and clamp
counts unchanged. No sentence in `js/i18n.js` still tells the user the labels do not change.
  </done>
</task>

<task type="checkpoint:human-verify" gate="blocking-human">
  <name>Checkpoint 4: Pattern review, AND the persistence round-trip that has never run</name>
  <action>
Stop. Two separate things need the developer, and the second has been outstanding since
Stage B.

**(a) The label pattern, before it reaches 42 more plugins.** Show the O-Tapestop diff:
the `data-i18n` attributes in `index.html`, the `LABELS` block, the canon v2 `applyI18n`,
and every CSS fix the geometry diff forced with its reason. Ask specifically whether the
French labels read correctly INSIDE O-Tapestop's aesthetic — French runs 15-20% longer and
a label that technically fits can still be visually wrong at 9.5px.

**(b) THE C++ PERSISTENCE ROUND-TRIP. This has never been executed on any of the five
plugins.** Every claim that a language choice survives a session is REASONED, not measured:
it rests on `getStateAsXml()` taking its own `copyState()` and `setStateFromXml()` doing
`replaceState(ValueTree::fromXml(...))`, both read and confirmed in the source, and on
`isVoid()` plus `.toString()` being the only correct guard because the XML round-trip
rebuilds every property as a `var` over the attribute string. It is a strong inference. It
is not a measurement, and it is the highest-value manual check on the list.

The script, on O-Tapestop, by hand:
1. Insert the plugin, pick Français, confirm labels and tooltips are French.
2. Save the session. Quit the DAW completely.
3. Reopen the session. **Still French?**
4. Load a factory preset. **Still French, and did the preset's parameters load?**
5. Add a FRESH instance. **English immediately — no blank frame, no flash of French?**

Also outstanding and worth doing while the DAW is open: the real
`Juce.getNativeFunction('getUiLanguage')` round trip through WKWebView has only ever been
exercised against a stub.

If step 3 fails, STOP — the same persistence code is in all five shipped plugins and the
fix belongs before replication, not after.

**Named deferral, carried not closed:** Windows/WebView2 is blocked on hardware. The repo
already carries this deferral verbatim in `.github/workflows/ci-tests.yml:140-148`. The
i18n-specific Windows risk is that WebView2 font metrics differ from WebKit's, so a French
label measured as fitting on macOS could clip on Windows. Nothing in this plan retires it.
  </action>
  <done>Developer has approved or amended the label pattern, and has run the persistence round-trip by hand and reported the result.</done>
</task>

<task type="auto" tdd="false">
  <name>T11 (Stage G): Retrofit gate-hardening — O-Octagon</name>
  <files>plugins/O-Octagon/Source/ui/public/js/{i18n.js,app.js}, plugins/O-Octagon/Source/ui/public/index.html, plugins/O-Octagon/Source/ui/public/css/styles.css, plugins/O-Octagon/tests/{ui_frontend_check.js,i18n-states.json}, plugins/O-Octagon/CHANGELOG.md, plugins/O-Octagon/CMakeLists.txt</files>
  <read_first>plugins/O-Octagon/tests/ui_frontend_check.js — sections 2 (init ordering), 6 (HTML-authored labels / the textContent whitelist) and 9/21 (module registry set-equality)</read_first>
  <action>
Same retrofit as T10, under the strictest static gate in the repo, and against the plugin
with by far the most visible-text attributes.

**O-Octagon carries 78 `aria-label` attributes — 45% of the repo's total** (fact 2). It is
the only plugin where attribute localization is the dominant job rather than a footnote,
and it is why this plugin comes second rather than being batched.

1. **Section 6 is the whole point of this stage.** It asserts that HTML-authored labels are
   never erased by a `textContent` write, and it carries an explicit whitelist of receivers
   whose comment says it is reviewed when it grows. Canon v2 makes `applyLabel` a
   `textContent` writer for every `[data-i18n]` element on the page. **Do not simply grow
   the whitelist by 78 entries** — that empties the assertion of meaning. Extend section 6
   to understand the `dataset.label` mirror: a write is legitimate when the element carries
   `data-i18n` and the written value equals `dataset.label`. That is a stronger assertion
   than the whitelist, and it is the one the rest of the rollout needs.
2. **Section 2 is binding.** `init();` must remain the literal last statement of `app.js`
   with no module-level declaration after it. Canon v2 adds `applyLabel` and `setLabel` to
   the block, which already sits above `init()`; the hoisted import stays the only new
   top-level form. Confirm rather than assume.
3. **Check every gate section for a wrong-shaped assumption before treating a failure as
   real.** Three instances in Stages C-D, all reporting violations of rules the code was
   obeying: section 9's double-quote-only import scan, `check-i18n`'s `js/app.js`
   assumption, and O-ReverseDelay's D13 substring search. Section 6's whitelist is the
   fourth candidate.
4. Fix the false `lang-select` sentence at `js/i18n.js:96-98` (fact 9), both languages.
5. Inventory, `data-i18n` patch, `data-i18n-aria` for all 78, canon v2, then the label gate
   and its fixes. 1100 x 720 is the roomiest frame among the five, so expect fewer geometry
   findings here than on O-Tapestop — if there are MORE, something is wrong with the tool
   or with the attribute handling.
6. Re-run every gate in `tests/`: `ui_frontend_check.js`, `ui_layout_check.js`, and the two
   C++ targets `O-Octagon-geometry-test` and `O-Octagon-render-test`, which are the ones CI
   actually runs per commit — build them from a fresh `-DOUARICON_BUILD_TESTS=ON` configure
   rather than assuming.
7. Version bump, CHANGELOG, PLUGINS.md, build-and-install, staging and commit discipline
   exactly as T10 step 10.
  </action>
  <verify>
    <automated>node plugins/O-Octagon/tests/ui_frontend_check.js &amp;&amp; node plugins/O-Octagon/tests/ui_layout_check.js &amp;&amp; node scripts/check-i18n.js --plugin O-Octagon &amp;&amp; node scripts/check-ui-labels.js --plugin O-Octagon &amp;&amp; ./scripts/build-and-install.sh O-Octagon</automated>
  </verify>
  <done>
All 43 sections of `ui_frontend_check.js` pass, with section 6 rewritten to the
`dataset.label` form and confirmed by negative control to FAIL when a `[data-i18n]`
element's `textContent` is made to disagree with its `dataset.label`. `ui_layout_check.js`
passes at 1100 x 720 — the label work did not disturb the layout it asserts. All 78
`aria-label`s resolve in both languages. The geometry diff reports zero non-label elements
moved. Both C++ targets pass.
  </done>
</task>

<task type="auto" tdd="false">
  <name>T12 (Stage H): Retrofit the remaining three — MBC, O-Bitrot, O-ReverseDelay</name>
  <files>plugins/O-{MultiBandCompressor,Bitrot,ReverseDelay}/** (per-plugin, one commit each)</files>
  <action>
Three plugins, one path-scoped commit each, in this order — each brings a distinct shape
the rollout needs proven before Stages I-L.

**1. O-MultiBandCompressor (120 text nodes, 900 x 640) — the `setLabel` proof.**
It is one of only nineteen plugins with JS-written prose, and it already owns the composed
`{token}` shape with nested-key `vars` resolution that Stage B settled. Its band labels are
the case where a label must be BOTH composed and re-rendered on a language change, which is
exactly what `setLabel` writing `data-i18n-vars` exists for. Prove it here.
It has NO UI gate in `tests/` — only the preset render harness — so `check-ui-labels.js` is
its only geometry evidence. Fix the false `lang-select` sentence at `js/i18n.js:295`.
**Known and pre-existing: its `preset-list` tooltip says "17 parameters" and v1.5.0 added
an 18th.** Stage C deliberately moved that stale count verbatim and flagged it. This is a
label-and-copy commit on that same file, so fix it now and say so in the CHANGELOG.

**2. O-Bitrot (90 text nodes, 900 x 740) — the inline-module proof.**
Its controller is one inline `<script type="module">` in `index.html`, not a `js/app.js`.
`check-i18n.js` already had to learn this in Stage D and the extractor was built for it in
T7, but canon v2 lands in an inline module here for the first time. The depth-adjusted
import specifier differs; the canon BODY does not, and the body is the part 43 hand-copies
can drift in. Fix the false sentence at `js/i18n.js:73`.
41 of its 53 tooltip anchors are idless wrappers — expect its labels to be similar, so the
extractor's generated key scheme gets its hardest test here.

**3. O-ReverseDelay (56 text nodes, 940 x 768) — the constrained-bridge proof.**
**D13 stands: no `setTooltipsEnabled`, ever, and no hover-help row in its popover.** The
assertion guarding it was tightened in Stage D to match a real registration rather than any
mention of the name, so comments explaining the absence are safe; do not loosen or
re-tighten it. Its `ui_frontend_check.js` also carries the rewritten anchor-coverage
assertion — every anchor bound, resolving to a key with both an `en` and an `fr` entry.
Extend that same shape to labels: every `data-i18n` key resolves and carries both
languages. That assertion found real drift in Stage D (two v1.8.0 knobs never added), so it
earns its extension. Fix the false sentence at `js/i18n.js:87`.
It needs `tests/i18n-states.json` for the sync/free mode swap, which hides one of two TIME
controls at any moment.

**Every one of the three:** inventory, review the UNSURE rows, English moved verbatim and
compared back mechanically, canon v2, label gate, fix what it names, every existing gate in
that plugin's `tests/` re-run, version bump, CHANGELOG, PLUGINS.md, build-and-install, and
the T10 step-10 staging and commit discipline. Verify each commit with `git show --stat`
before moving to the next plugin — Stage D caught another session's PLUGINS.md row in a
commit exactly this way.
  </action>
  <verify>
    <automated>for P in O-MultiBandCompressor O-Bitrot O-ReverseDelay; do node scripts/check-i18n.js --plugin $P || echo "FAIL i18n $P"; node scripts/check-ui-labels.js --plugin $P || echo "FAIL labels $P"; done; node plugins/O-ReverseDelay/tests/ui_frontend_check.js; for P in O-Bitrot O-ReverseDelay; do node plugins/$P/tests/ui_tooltip_clamp_check.js || echo "FAIL clamp $P"; done; node scripts/check-i18n.js --strict-v2</automated>
    <human-check>On MBC in the DAW: switch to Français and confirm the per-band labels compose correctly in French — the band name and the control name both translate, and neither is stranded in the other language.</human-check>
  </verify>
  <done>
All five shipped plugins report **canon v2**; `check-i18n.js --strict-v2` exits **0** for
the localized set. `check-ui-labels.js` exits 0 for all five with zero non-label geometry
shifts. Both clamp gates still exit 0 — not 77 — with Stage D's counts. O-ReverseDelay's
D13 assertion is green and untouched. No plugin's `i18n.js` still claims its labels do not
change. **This is the recommended partial-ship point.**
  </done>
</task>

<task type="checkpoint:human-verify">
  <name>Checkpoint 5: The French LABEL geometry verdict — and the ship decision</name>
  <action>
Stop and report the measured result before it is replicated across 38 plugins.

Stage D produced the tooltip verdict — French cost three extra vertical flips on one plugin
and zero extra clamps anywhere, because tooltips WRAP under a fixed `max-width`. **Labels do
not wrap the same way, and this is the first hard data on whether that matters.** Report,
across all five frames from 860 x 580 to 1100 x 720:

- how many labels were longer in French, and by how much at the worst
- how many non-label elements the geometry diff named as MOVED, before any fix
- what each fix was, and whether any needed a layout change rather than a CSS tweak
- how many labels were never visible in any driven state, so the coverage hole is stated
  rather than implied

**If any plugin needed a layout change rather than a tweak, say so plainly** — D-04 forbids
auto-shrink and short variants, so a layout change is the only remaining move and it is a
design decision, not a task step. Present it; do not pick it.

**Then the ship decision.** Five plugins fully localized rather than half-localized, the
geometry risk measured rather than argued, the persistence round-trip finally executed by
hand, and the tooling in place. Stages I-L are replication over 38 plugins at an honest
1.2M-3.4M tokens. Recommend stopping here unless the geometry findings were uniformly
trivial — if the five frames each needed several fixes, 38 more will need proportionally
more, and that is worth knowing before committing to them.
  </action>
  <done>Developer has seen the measured label-geometry result across five frames and either approved Stage I or chosen to ship at five plugins.</done>
</task>

<task type="auto" tdd="false">
  <name>T13 (Stage I): The remaining 9 data-tip plugins — tooltips AND labels in one pass</name>
  <files>plugins/O-{Contrabass,Orbit,simpleGrain,simpleSampler,simpleSubtractive,simpleFM,simpleAdditive,simplePhysicalModelSynth,simpleBeatmaker}/** (per-plugin, one commit each)</files>
  <action>
These 9 already use the `data-tip` renderer, so their tooltip copy MOVES to `i18n.js`
unchanged. Under the expansion each plugin gets tooltips AND labels in the SAME commit —
splitting them would ship a half-localized plugin twice, which is the state the five
shipped ones are being rescued from.

**Enumerated, one path-scoped commit per plugin.**

*Batch I1 — the two with gates or an unusual root. Do these first, alone:*

| Plugin | Root | Frame | Tips | Text | Attrs | Note |
|---|---|---|---|---|---|---|
| O-Contrabass | `Source/ui/public` | 1000x650 | 44 | 61 | **47** | `tests/ui_frontend_check.js:123` asserts `called.size === 34 && registered.size === 34` -> **36**. Second-highest attribute count in the repo. **Check its gates for a double-quote-only import scan BEFORE bumping the count** — that wrong-shaped assumption cost a false failure in Stage C. Has the tooltips bridge; the toggle moves into the popover. |
| O-Orbit | **`Resources/ui`** | 800x600 | 32 | 57 | **38** | Serves `Resources/ui/` per `CMakeLists.txt:54-62`; its `Source/ui/public/` is NOT embedded — write only into the served root. Its `configure_file` vendored preset-manager copy already writes into `Resources/ui/js/modules/`, so the serve tool must overlay it. Has the tooltips bridge. |

*Batch I2 — the `simple*` family, seven plugins, no tooltip gates, no bridge toggle:*

| Plugin | Frame | Tips | Text | Attrs |
|---|---|---|---|---|
| O-simpleSubtractive | 1180x820 | 33 | 67 | 5 |
| O-simpleGrain | 900x760 | 34 | 60 | 5 |
| O-simpleFM | 760x980 | 25 | 54 | 10 |
| O-simpleSampler | 980x720 | 34 | 52 | 6 |
| O-simpleAdditive | 860x930 | 24 | 48 | 3 |
| O-simpleBeatmaker | 1060x900 | 20 | 47 | 1 |
| O-simplePhysicalModelSynth | 1040x860 | 24 | 46 | 14 |

None has the tooltips bridge, so none gets a hover-help row — language selector alone. All
seven have a `tests/render-harness/`; none has a tooltip gate. All seven are tall, roomy
frames, so the geometry risk here is the lowest in the rollout.

**Per plugin, in this order:**
1. `node scripts/i18n-extract.js --plugin <Name>`; review every `UNSURE` and `READOUT` row.
2. `grep -rn 'setVisible' plugins/<Name>/Source/` — abort and report if it targets the web
   view. A hidden `WebBrowserComponent` drops native-function completions and the one-shot
   language pull never settles.
3. `grep -n -i version plugins/<Name>/CMakeLists.txt` — several declare the version through
   a `set(<PLUGIN>_VERSION ...)` variable consumed by both the plugin and its render
   harness. Bump the variable, never a second copy.
4. Tooltip copy moved verbatim into `I18N`; labels added; `data-i18n` attributes applied;
   attributes keyed; native `title=` deleted.
5. Canon v2 block, called from the plugin's existing deferred init inside try/catch.
6. Gear popover, styled inside that plugin's own visual system — it is not one uniform
   widget pasted in unchanged.
7. C++ language pair and persistence, mirroring whichever idiom that plugin already uses
   for its own non-parameter state.
8. `i18n.js` into `SOURCES` + a `getResource()` branch + the import, all in the same commit.
9. French drafted, every entry `reviewed: false`.
10. Both gates, then every gate in that plugin's own `tests/`, then build-and-install,
    then the T10 step-10 staging and commit discipline, then `git show --stat`.
  </action>
  <verify>
    <automated>node scripts/check-i18n.js --strict-v2 &amp;&amp; node plugins/O-Contrabass/tests/ui_frontend_check.js &amp;&amp; for P in O-Contrabass O-Orbit O-simpleGrain O-simpleSampler O-simpleSubtractive O-simpleFM O-simpleAdditive O-simplePhysicalModelSynth O-simpleBeatmaker; do node scripts/check-ui-labels.js --plugin $P || echo "FAIL $P"; done</automated>
    <human-check>Spot-check one plugin from each batch in the DAW: switch to Français and confirm labels, tooltips and any status text are all French, then reopen the session and confirm the choice held.</human-check>
  </verify>
  <done>
All 9 pass `check-i18n.js --strict-v2` and `check-ui-labels.js` with zero non-label
geometry shifts. O-Contrabass's frontend gate passes at a bridge surface of 36 and its 47
attributes resolve in both languages. All 14 `data-tip`-convention plugins are now fully
localized. No plugin retains a native `title=` on a control that has a `data-tip`.
  </done>
</task>

<task type="auto" tdd="false">
  <name>T14 (Stage J): Port the 7 second-renderer plugins, then localize them fully</name>
  <files>plugins/O-{FreqPulse,Polystutter,SpectralShaper,Lyrica,IntonationPad,Gain,Marimba}/** (per-plugin, one commit each)</files>
  <read_first>plugins/O-ReverseDelay/Source/ui/public/js/app.js — the measure-then-pin renderer being ported IN; plugins/O-Polystutter/Source/ui/public/index.html:1670-1757 — the never-measures positioner being ported OUT</read_first>
  <action>
These 7 are PORTED onto the measure-then-pin renderer, not localized in place. Their
current positioner never measures — it carries a hard-coded approximate tooltip height, a
hard-coded tooltip width, and two hard-coded viewport literals — so it is already wrong
before French makes strings taller. After this stage there is ONE tooltip renderer repo-wide.

**Enumerated, one path-scoped commit per plugin, hardest first:**

| # | Plugin | Root | Frame | Tips | Text | JS prose | Port note |
|---|---|---|---|---|---|---|---|
| 1 | O-FreqPulse | **`Resources/ui`** | 850x550 | 12 | 21 | 2 + **3 composed** | **Do first.** Synthesises its tip text at runtime with interpolated band names, so it needs the `{token}` entries and the `setLabel` vars path proven on MBC in Stage H. Has the tooltips bridge. |
| 2 | O-Polystutter | `Source/ui/public` | 1000x690 | 102 | 96 | 2 composed | The largest port and the source of the hard-coded literals. Its renderer is inline in `index.html`; extract it to `js/app.js` alongside the port. Its `.tooltip` cap is **220**, not 230 — parse it, never mirror it. |
| 3 | O-Lyrica | **`Resources/ui`** | 700x450 | 43 | **181** | 5 + 5 composed | Second-heaviest text load in the repo and the heaviest in this stage. `CMakeLists.txt:88-96` has **NO `NAMESPACE`** on its binary-data target and relies on being the only one — add `i18n.js` to the existing target, never a second target. Has the tooltips bridge. Its JS prose includes preset-group headings (`Factory`, `Custom`), which are chrome and DO localize, and tuning names like `12-TET Standard`, which are identifiers and do NOT — an `I18N_EXEMPT` entry with that reason. |
| 4 | O-SpectralShaper | **`Resources/ui`** | 700x500 | 25 | 25 | 0 | Already measures but positions against `#app` rather than the viewport, so the port changes the reference frame. Has the tooltips bridge AND a committed `tests/ui-stub/juce-stub.js` that needs the two language names. |
| 5 | O-IntonationPad | `Source/ui/public` | 800x500 | 37 | 46 | 2 | No toggle bridge; language selector alone. |
| 6 | O-Marimba | `Source/ui/public` | 600x400 | 15 | 40 | **7** | Third-highest JS prose count in the repo, and they are mode names (`Edge`, `Center`, `Shimmer`, `Focused`, `Warm`, `Bright`) written from script — every one needs `setLabel`. Has the tooltips bridge. **600x400 is a tight frame.** |
| 7 | O-Gain | `Source/ui/public` | **350x500** | 23 | 41 | 2 | **The narrowest frame in the repo.** 41 labels in 350px is where French label overflow is most likely to demand a real layout answer rather than a tweak. No toggle bridge. |

**Per plugin:**
1. **Split the existing single-string tooltip copy** on the first `": "` to recover title and
   body — it is already authored in that shape. Expect a handful that do not split cleanly;
   hand-split those and list them in the commit message rather than letting a colon slide
   into the body.
2. **Port the renderer** from O-ReverseDelay: fixed positioning, width released then
   measured then PINNED before `left` is applied, vertical flip, horizontal clamp, arrow
   offset recomputed after the clamp, the dwell delay, the enabled gate. **Delete the old
   positioner and its four hard-coded literals entirely** — leaving both is how two
   renderers came to exist.
3. **Port the CSS.** Carry each plugin's own `max-width` value across unchanged; do not
   normalise them to one number. The gate parses the value, so a per-plugin cap is fine and
   a mirrored literal is not.
4. Then the full contract: labels, attributes, canon v2, popover, C++ pair, persistence,
   the four-place rule in one commit, French, version, CHANGELOG, PLUGINS.md, both gates,
   build-and-install, staging and commit discipline.
5. **Verify the port by RENDERING, not by inspection.** `scripts/serve-ui.js` picks port 0
   for a reason — a server on an already-taken port silently serves another session's files.
6. **O-Gain gets extra scrutiny.** If 350px cannot hold the French labels without a layout
   change, STOP and report it rather than picking a fix: D-04 rules out the two cheap
   answers and what remains is a design decision.
  </action>
  <verify>
    <automated>node scripts/check-i18n.js --strict-v2 &amp;&amp; for P in O-FreqPulse O-Polystutter O-Lyrica O-SpectralShaper O-IntonationPad O-Marimba O-Gain; do node scripts/check-ui-labels.js --plugin $P || echo "FAIL $P"; done; grep -rn 'tooltipHeight' plugins/O-*/Source/ui/public plugins/O-*/Resources/ui 2>/dev/null | grep -v '^\s*//' | wc -l</automated>
    <human-check>For each of the 7: hover a long tip near the right edge and near the top edge, in both languages, and confirm the tip stays fully on screen with its arrow still pointing at its anchor.</human-check>
  </verify>
  <done>
The old-positioner literal grep returns **0**. No served markup carries the second
renderer's tooltip attribute. All 7 render through the measure-then-pin runtime and pass
both gates with zero non-label geometry shifts. All 21 plugins that ever had tooltip copy
are fully localized. **ONE tooltip renderer repo-wide — the natural feature boundary and the
third stop point.**
  </done>
  <reversibility rating="one-way">Deleting the second renderer is one-directional: the 7 plugins' old positioners are gone, not disabled.</reversibility>
</task>

<task type="auto" tdd="false">
  <name>T15 (Stage K): The 21 remaining bare plugins — LABELS ONLY, no tooltip authoring</name>
  <files>plugins/O-{AnalogEQ,AnalogSaturation,Bass,Bassoon,Bells,Bowed,Chorus,Comp,Detune,DigiDelay,Emulator,Formant,Freeze,GrainScatter,MicrotonalSampler,Reed,SimpleReverb,Texture,TextureForge,Tremolo,Wind}/** (per-plugin, one commit each)</files>
  <action>
These 21 have no tooltip copy today — only stray native `title=` attributes. **This stage
localizes their existing visible text and does NOT author hover-help prose.** That
authoring is Stage M, and the recommendation in the STAGING section is to defer it to its own task.
Nothing here can regress a tooltip, because there are none.

Each plugin gets: labels, attributes, JS-written prose, the language selector in a gear
popover, the C++ pair and persistence, and both gates. It does NOT get an `I18N` tooltip
section — `TIP_BINDINGS` is empty and `check-i18n` assertion 2 is vacuously satisfied,
which the gate must report as "0 tips bound" rather than silently pass.

**Batches, cheapest first so the pattern is cheap to correct, and the tight frames isolated:**

*Batch K1 — small text, roomy or moderate frames (7 plugins):*
O-AnalogSaturation (14 text, 600x450), O-Texture (16, 800x600), O-Emulator (17, 620x430),
O-Freeze (23, 550x530), O-TextureForge (25, 900x600), O-Bassoon (33, `Resources/ui`,
900x600), O-Detune (34, 600x480).
O-Emulator has a `tests/` dir — re-run its gates. Its parameters come from a local factory
lambda, which is why the param-dump exists; irrelevant to labels, relevant if Stage M runs.

*Batch K2 — THE TIGHT FRAMES. These are the geometry risk in this stage (5 plugins):*
| Plugin | Frame | Text | Why it is here |
|---|---|---|---|
| O-Chorus | **700x125** | 15 | The shortest frame in the repo. 125px of vertical space; a label that gains a line has nowhere to go. |
| O-DigiDelay | **700x196** | 16 | Second shortest. |
| O-AnalogEQ | **920x220** | 25 | Wide but only 220 tall, and it carries 3 `nowrap` + 1 `ellipsis` — the clip risk is real here rather than theoretical. |
| O-Bass | **420x320** | 12 | Narrow. |
| O-SimpleReverb | **500x350** | 20 | Narrow. |
**Do this batch one plugin at a time and report the geometry diff for each before moving
on.** If two of the five need layout changes, stop and raise it — that is a pattern, not an
incident, and it changes what Stages K3-K4 will cost.

*Batch K3 — medium (5 plugins):*
O-Comp (19, 620x360), O-Tremolo (19, 600x400), O-Bowed (50, `Resources/ui`, 900x600),
O-Reed (58, `Resources/ui`, 900x600), O-GrainScatter (73, 900x800).
O-Bowed has a `tests/` dir — re-run its gates.

*Batch K4 — the volume (4 plugins, one per dispatch):*
| Plugin | Root | Frame | Text | Attrs | JS prose | Note |
|---|---|---|---|---|---|---|
| O-Wind | `Resources/ui` | 900x600 | 65 | 3 | 1 | |
| O-Bells | `Resources/ui` | 800x600 | 84 | 1 | 1 + 2 composed | |
| O-Formant | `Source/ui/public` | 800x600 | 109 | 5 | 1 composed | 6 grid-column declarations — the layout most likely to shift. |
| O-MicrotonalSampler | **`Resources/ui`** | 900x640 | 126 | 14 | **6 + 8 composed** | **The hardest plugin in this stage.** Serves `Resources/ui/` per `CMakeLists.txt:80-99`; its `Source/ui/public/` is a build-time JS-copy staging dir and is NOT embedded — write only into the served root. It has the most composed strings in the repo AND the only inline English pluralization (fact 4): author around the inflection per contract section 6 rather than building a plural engine. Its status messages (`Sample folder not found`, `Drag-dropped samples not embedded`) are user-facing prose and matter more to a French user than any knob label. Also the plugin with the most `nowrap` + `overflow: hidden` rules, so run its geometry diff carefully. |

**Per plugin: the same ten steps as T13**, minus the tooltip-copy move. Delete every native
`title=` per contract section 4; where a `title=` is an element's only help its text becomes
`data-i18n-aria`, and no new prose is invented.
  </action>
  <verify>
    <automated>node scripts/check-i18n.js --strict-v2; for P in O-AnalogEQ O-AnalogSaturation O-Bass O-Bassoon O-Bells O-Bowed O-Chorus O-Comp O-Detune O-DigiDelay O-Emulator O-Formant O-Freeze O-GrainScatter O-MicrotonalSampler O-Reed O-SimpleReverb O-Texture O-TextureForge O-Tremolo O-Wind; do node scripts/check-ui-labels.js --plugin $P || echo "FAIL $P"; done</automated>
    <human-check>Per batch, open one plugin in the DAW and confirm every label, heading and button caption switches to French, that nothing clips or overlaps, and that the value readouts are unchanged.</human-check>
  </verify>
  <done>
All 21 pass both gates with zero non-label geometry shifts, including all five tight
frames. `check-i18n.js --strict-v2` exits 0 across 42 localized plugins. Every plugin's
`TIP_BINDINGS` state is reported explicitly rather than passing silently. No native `title=`
survives in any served markup. O-MicrotonalSampler's status messages and composed strings
render correctly in both languages at every count, including zero.
  </done>
</task>

<task type="auto" tdd="false">
  <name>T16 (Stage L): O-Prism — alone</name>
  <files>plugins/O-Prism/** (one commit; split into two only if a single commit cannot be verified)</files>
  <action>
**The largest single item in the project, batched with nothing.**

- **247 static text nodes** — the most of any plugin, and 2.6x the next-largest in Stage K.
- **173 runtime parameters**, confirmed by the Stage A param-dump. A regex over its source
  finds zero usable IDs because they are string-concatenated and never appear as literals.
  Only the runtime walk enumerates them.
- **9 JS-written prose strings** — the most in the repo. `Sync`, `Free`, `Free Run`,
  `Retrig`, `Import WAV...`, `Manage...` are mode captions and menu items written from
  script; every one needs `setLabel`, and every one is a caption a state updater rewrites,
  so this is where the `dataset.label` mirror earns its keep.
- **1200 x 800** is the roomiest frame in the repo, which is the one thing in its favour.
- It is **one of the two plugins in the repo with an editable value-entry knob family**.
  **D-03 exists because of this plugin**: localizing a decimal separator would require that
  parser to accept both, and a silent value-entry break is worse than a period where a comma
  belongs. **Do not touch any readout, any formatter, or any parser.** The gate's `READOUT`
  classification must be reviewed row by row here, not skimmed.

Steps as T13, with three additions:
1. Run the param-dump first for the inventory even though no tooltip prose is authored —
   it is the only reliable map of what this plugin's controls actually are, and the
   extractor's key generation benefits from real parameter names.
2. Review the extractor's classification of all 247 rows by hand, in sections. This is the
   one plugin where skimming the inventory would ship a translated number.
3. If a single commit cannot be verified end to end, split into exactly two — markup and
   table first, then the JS `setLabel` conversions — and verify each. Do NOT split further:
   the four-place rule means `i18n.js`, `SOURCES`, `getResource()` and the import land
   together or the page 404s at runtime.
  </action>
  <verify>
    <automated>node scripts/check-i18n.js --plugin O-Prism &amp;&amp; node scripts/check-ui-labels.js --plugin O-Prism &amp;&amp; node scripts/check-i18n.js --strict-v2 &amp;&amp; ./scripts/build-and-install.sh O-Prism</automated>
    <human-check>In the DAW: switch O-Prism to Français, then type a value into an editable readout and confirm it is accepted exactly as it was in English. Switch modes and confirm the mode captions are French rather than stranded in English. Confirm every readout still reads with a period.</human-check>
  </verify>
  <done>
All 43 plugins carry an `i18n.js` under their actually-served UI root, present in
`juce_add_binary_data SOURCES`, in a `getResource()` branch, and imported by the controller
module. `check-i18n.js --strict-v2` exits 0 across all 43 and can now become the default;
canon v1 can be deleted from `i18n-canon.js`. Every editable readout on O-Prism still
accepts what it displays, in both languages.
  </done>
</task>

<task type="checkpoint:decision">
  <name>Checkpoint 6: The Stage M scope decision</name>
  <action>
Stop. All 43 plugins are label-localized. The remaining question is the one the brief asked
to be flagged rather than decided unilaterally.

**Should English hover-help prose be authored for the 22 plugins that have none?**

Report the real number, not the planning estimate: sum the param-dump row counts across the
22 and state it. The original figure of ~520 was a regex estimate, and the param-dump has
already proven such estimates low — O-Prism alone came in at 173 against an estimate of 81.

**Recommendation: defer to its own task.** The reasoning is in the STAGING section: the volume grew
rather than shrank, it is content work verified by a human who knows the DSP rather than by
a gate, and nothing depends on it. A French user of O-Bass would get French labels and no
hover-help, which is exactly the help an English user of O-Bass gets today — a consistency
gap, not a regression.

**Against deferring:** 21 plugins with rich hover-help and 22 with none is a visible
inconsistency in a suite, and the param dumps and inventories are freshly generated right
now, which is the cheapest this work will ever be.

Also report the outstanding French review worklist: `node scripts/check-i18n.js` prints
per-plugin unreviewed counts, and every French string in the repo is still a machine draft
that no native speaker has read. That is a separate task from Stage M and it gates any claim
that the plugins are actually usable in French.
  </action>
  <done>Developer has decided whether Stage M runs here, becomes its own task, or is dropped.</done>
</task>

<task type="auto" tdd="false">
  <name>T17 (Stage M, CONDITIONAL on Checkpoint 6): Author English hover-help for the 22 bare plugins</name>
  <files>plugins/&lt;the 22&gt;/&lt;uiroot&gt;/js/i18n.js, plugins/&lt;the 22&gt;/&lt;uiroot&gt;/index.html (per-plugin, one commit each)</files>
  <action>
Only if Checkpoint 6 approves. Content work, not engineering, and it is the last stage for
that reason — every gate it needs is already green and every plugin it touches is already
localized, so a stopped batch leaves the repo consistent.

Per plugin, the skeleton is machine-generated and the prose is hand-written:
1. Wire and run the param-dump: three lines in that plugin's `CMakeLists.txt` behind the
   existing `OUARICON_BUILD_TESTS` option, then redirect the TSV into
   `plugins/<Name>/.planning/params.tsv`. Authoritative even where IDs are concatenated or
   parameters come from a factory lambda — the two cases that defeat static parsing.
2. One `I18N` tooltip entry per parameter, with the title from the display name and the
   body hand-written: what the control does, when to reach for it, ending with the range
   and unit from the dump. Where the dump's label column is empty the range must be phrased
   from the UI's own formatter — read how `app.js` renders that readout, never invent it.
   O-Chorus has zero unit strings on all 8 of its parameters and is the worked example.
3. Bind `TIP_BINDINGS` to the ids the UI already uses. Any surviving native `title=` on
   those controls was already deleted in Stage K, so there is no double tooltip.
4. Draft French for the new entries, `reviewed: false`.
5. Both gates, build-and-install, version, CHANGELOG, PLUGINS.md, staging and commit
   discipline as T10 step 10.

Order: the batches from Stage K, cheapest first, and one plugin per dispatch for the four
in K4. Stopping between plugins is safe.
  </action>
  <verify>
    <automated>node scripts/check-i18n.js --strict-v2; echo "--- plugins with zero bound tips ---"; node scripts/check-i18n.js 2>&amp;1 | grep -c 'tips bound: 0'</automated>
    <human-check>Per batch, open one plugin in the DAW: every knob and control has a tooltip, the copy is accurate about what the control does, and the stated range matches what the readout actually shows.</human-check>
  </verify>
  <done>
Every plugin with an authored batch reports a non-zero bound-tip count. `check-i18n.js`
exits 0 across all 43 and prints the full unreviewed-French worklist as a native-speaker
handover.
  </done>
</task>

</tasks>

<threat_model>
No new external dependencies — no npm, pip or cargo install — so the package legitimacy
gate does not apply. Playwright is a pre-existing tool dependency and the gates resolve it
themselves, exiting 77 when they cannot.

| Boundary | Description |
|---|---|
| i18n table -> DOM label | Machine-drafted French is now written into the page BODY, not only into tooltip attributes |
| `data-i18n-vars` -> `JSON.parse` | A per-element JSON string is parsed on every language change |
| WebView JS -> C++ | `setUiLanguage` accepts an arbitrary string from the page |
| Session XML -> C++ | `uiLanguage` is read back from a user-editable saved-session file |
| Generic stub -> rendered page | A test-only bridge substitutes for the real native surface on 38 plugins |

| ID | Category | Component | Severity | Disposition | Mitigation |
|---|---|---|---|---|---|
| T-ieq-01 | Tampering | localized string -> label node | medium | mitigate | `applyLabel` writes `textContent`, never `innerHTML`. `check-i18n` assertion 9 already rejects any `innerHTML` reference in `i18n.js` and any `<` inside a string literal, and now covers `LABELS` as well as tooltip entries. A line break uses `\n` plus CSS `white-space: pre-line`, never a markup tag. |
| T-ieq-05 | Denial of Service | `JSON.parse(el.dataset.i18nVars)` | medium | mitigate | Parsed inside try/catch with a warn-and-continue fallback. A malformed vars attribute on one element must not take the whole label sweep — and therefore the whole page's chrome — down. This is `pattern_module_toplevel_init_tdz` one layer up. |
| T-ieq-06 | Information Disclosure | generic stub in a shipped tree | low | mitigate | `scripts/ui-stub/` lives outside every plugin's UI root and is never listed in a `juce_add_binary_data SOURCES` block. `check-i18n` assertion 8 derives the embedded set from CMake, so a stub that crept into SOURCES would fail set-equality on the plugins that assert it. |
| T-ieq-07 | Spoofing | generic stub masking a real bridge break | medium | mitigate | The generic stub resolves unknown native-function names, which is the OPPOSITE of the hand-written stubs' rejection behaviour. That is correct for a rendering tool and wrong for a census, so the five plugins with a hand-written stub keep using it, and no native-function count assertion is ever run against the generic one. Stated in `scripts/ui-stub/README.md`. |
| T-ieq-02 | Tampering | `setUiLanguage` argument | low | mitigate | C++ maps anything that is not the French code to English; JS falls back to English for anything not in `LANGUAGES`. Unchanged from Stages A-D. |
| T-ieq-03 | Tampering | `uiLanguage` restored from session XML | low | mitigate | Guarded by `isVoid()`, read via `.toString()`, clamped by the same codec. A hand-edited or corrupt value degrades to English. Unchanged. |
| T-ieq-04 | Denial of Service | module evaluation | high | mitigate | `applyI18n` is called only from deferred init inside try/catch, `tr()` returns a fallback for a missing key rather than throwing, and `i18n.js` has no top-level statement — enforced by `check-i18n` assertion 7 and, on O-Octagon, by its own gate section 2. |
</threat_model>

<verification>
Repo-wide, after any stage:

```bash
node scripts/check-i18n.js                     # coverage, drift, exemptions, canon split
node scripts/check-i18n.js --strict-v2         # default only after Stage L
node scripts/boot-all-uis.js                   # all 43 pages still render
for P in $(ls plugins); do node scripts/check-ui-labels.js --plugin $P; done
node plugins/O-Octagon/tests/ui_frontend_check.js
node plugins/O-Octagon/tests/ui_layout_check.js
node plugins/O-ReverseDelay/tests/ui_frontend_check.js
node plugins/O-Contrabass/tests/ui_frontend_check.js
for P in O-ReverseDelay O-Bitrot O-Tapestop; do node plugins/$P/tests/ui_tooltip_clamp_check.js; done
```

Exit **77** from any Playwright gate means the tool was unresolvable and **nothing was
verified**. It is deliberately distinct from 0. Treat it as a failure to verify, never as a
pass.

After any merge that touched `PLUGINS.md`, run the union-merge duplicate check —
`grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d` — where empty output
means clean. Union merge duplicates ADJACENT rows even when each side edited only its own.

**Carried forward from Stages A-D, still open:**
1. The C++ persistence round-trip has never been executed on any plugin — **Checkpoint 4,
   blocking.**
2. The real `Juce.getNativeFunction` round trip has only ever run against a stub —
   Checkpoint 4.
3. The Standalone `.app` is stale on all five; `build-and-install.sh` builds VST3 and AU
   only. Do not judge a UI change there without rebuilding that target.
4. All 200 French entries are unreviewed machine drafts. `check-i18n.js` prints the
   worklist. This gates any claim that the plugins are usable in French, and it is a
   separate task from this one.
5. **Windows / WebView2 is a NAMED DEFERRAL, blocked on hardware**, matching the deferral
   the repo already carries at `.github/workflows/ci-tests.yml:140-148`. The i18n-specific
   risk is that WebView2 font metrics differ from WebKit's, so a French label measured as
   fitting on macOS could clip on Windows. Nothing in this plan retires it.
</verification>

<success_criteria>
- All 43 plugins carry an `i18n.js` under their **actually served** UI root, present in
  `juce_add_binary_data SOURCES`, in a `getResource()` branch, and imported by the
  controller module — which is an inline `<script type="module">` on at least one plugin.
- Every visible string in every served `index.html` is a `[data-i18n]` element, a keyed
  attribute, or an `I18N_EXEMPT` entry **with a reason**.
- Every prose string written to `textContent` from script goes through `setLabel`, so it
  re-renders on a language change through the SAME sweep as an HTML-authored label.
- **Zero native `title=` attributes** remain in any served markup.
- `dataset.label === textContent` for every `[data-i18n]` element after page init, after a
  language switch, and after a state-update pass.
- `check-ui-labels.js` passes for all 43 with **zero non-label elements moved** between
  English and French, and with the vacuity guard confirming French actually rendered.
- `check-i18n.js --strict-v2` exits 0 across all 43; canon v1 is deleted.
- The three tooltip clamp gates still pass for both languages with Stage D's counts.
- O-Octagon, O-ReverseDelay and O-Contrabass frontend gates pass at their bridge counts,
  with each ui-stub whitelist equal to its C++ surface.
- O-ReverseDelay's D13 assertion is green and untouched, and the plugin still has no
  hover-help toggle.
- **No `toFixed()` site, readout formatter or value-entry parser was modified (D-03), and
  no `createParameterLayout()` display name was modified (D-01).** Both verified by diff.
- Factory preset names are unchanged and every session still recalls its preset (D-02).
- Every plugin touched has a version bump, a CHANGELOG entry and an updated PLUGINS.md row,
  in a single path-scoped commit verified with `git show --stat`.
</success_criteria>

<output>
Per-plugin commits only. After each stage, report which plugins shipped, at what versions,
which gates ran with what exit codes, and — for every stage from F onward — **the geometry
diff result: how many non-label elements moved in French, and what each fix was.**
</output>
