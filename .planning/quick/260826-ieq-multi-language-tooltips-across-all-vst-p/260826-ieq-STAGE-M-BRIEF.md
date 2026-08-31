# Stage M executor brief — T17: hover-help for the 22 bare plugins

Read this in full before touching a file. It is the standing brief for all 22
dispatches across M1-M3; the dispatch prompt names only the plugin and its batch.

Read `260826-ieq-STAGE-K-BRIEF.md` first for the parts this file does not
repeat: the D-01 three-arm test, the `I18N_EXEMPT` contract, the build mutex,
the scratchpad namespacing, the commit discipline, and the carried traps. Where
the two disagree, THIS file wins.

---

## THE HEADLINE CORRECTION: this is not content work

T17 says "Content work, not engineering." **That is wrong, and it was measured
before the first dispatch.** All 22 plugins were scanned for a tooltip surface,
tooltip CSS and a hover handler:

```
for P in <the 22>; do
  grep -rl 'id="tooltip"' $ROOT ; grep -rn '\.tooltip {' $ROOT ;
  grep -rn 'closest("\[data-tip\]")' $ROOT
done
```

**All three are ZERO on all 22.** Not one of them has a tooltip renderer, a
tooltip surface, tooltip CSS, or a hover handler.

Canon v2's `applyI18n()` writes `data-tip-title` and `data-tip` **attributes**
onto the anchors named in `TIP_BINDINGS`. That is all it does. The thing that
reads those attributes and paints a surface is per-plugin code that lives
outside the canon, and on these 22 plugins **it does not exist**.

So authoring 72 tooltip bodies into `I18N` and binding them, with no other
change, ships **72 invisible strings and a green gate**. `check-i18n` would pass
(assertion 2 sees bindings > 0), `check-ui-labels` would pass (it has no tooltip
awareness at all), and `boot-all-uis` would pass (it counts `aria-label` and
`title`, never `data-tip`). That is the seventh instance in this task of a gate
certifying the absence of a thing it cannot see, and it is the one you are
standing in.

**Each plugin therefore gets a renderer as well as copy.** The renderer is
specified below, verbatim, so that 22 hand-copies do not become 22 dialects.

---

## The authority documents, in order

1. `260826-ieq-STAGE-K-BRIEF.md` — the standing brief. Everything it says that
   this file does not contradict still binds.
2. `260826-ieq-PLAN.md`
   - lines 195-299 — **CANONICAL CONTRACT V2**. Sections 1-8. Read every one.
     §4 (native `title=` is DELETED) and §5 / D-03 (readouts stay English) are
     the two that bite here.
   - lines 1253-1290 — **T17**, this task.
   - lines 741-800 — **T10**, whose step 10 staging and commit discipline this
     stage reuses.
3. `260826-ieq-SUMMARY.md` — the stage log. Read the **Stage L** section
   (search `# STAGE L`) and the **K4** section (`# STAGE K — BATCH K4`).
4. `scripts/i18n-canon.js` — canon v2, held as DATA. **Do not touch it.** This
   stage adds no canon change: `applyI18n()` already writes the tip attributes.
5. `scripts/param-dump/README.md` — the runtime parameter inventory.

## The reference implementations

- **The i18n table shape with tooltips:** `plugins/O-Tapestop/Source/ui/public/js/i18n.js`.
  Read the header comment and the `TIP_BINDINGS` block at the foot. Note the
  `[selector, key, wrapper]` triple: the selector finds an id'd child and
  `closest(wrapper)` walks back up to the cell the tip belongs on.
- **The renderer:** `plugins/O-simpleFM/Source/ui/public/js/app.js:379-462`
  (`setupTooltips`) and `css/styles.css:683-709`. **This is the family M1
  ports.** See "The renderer, specified" below.
- **Your own plugin's Stage K commit.** `git log --oneline -- plugins/<Name>`
  and read the `improve: <Name> ... the PAGE speaks French` commit. Its
  `i18n.js` header comment records every measured cliff and geometry pin on your
  page. That is the document that tells you where a tooltip has room.

---

## YOUR NUMBERS, MEASURED — and the param dumps are ALREADY RUN

The plan's `~520` parameters was low. The real repo-wide figure is **607 across
22 plugins**, established three independent ways that agree exactly: an `auval`
runtime walk of the installed binaries, calibrated against O-Prism's param-dump
(173 = 173); and, for M1, a fresh param-dump build from source.

**The orchestrator has already wired and run every M1 param-dump.** `build/` is
a shared resource and ten concurrent `cmake` invocations would corrupt it, so
this was done once, serially, in an isolated build directory. You will find in
your plugin's working tree, **uncommitted**:

| Path | What it is |
|---|---|
| `plugins/<Name>/.planning/params.tsv` | your authoritative parameter inventory |
| `plugins/<Name>/CMakeLists.txt` | +14 lines: `option(OUARICON_BUILD_TESTS ...)` and the `ouaricon_add_param_dump()` call |
| `plugins/<Name>/Source/PluginProcessor.cpp` | +15 lines: `#include "PluginEditor.h"` moved behind `#if JUCE_WEB_BROWSER` above `createEditor()`, with a `GenericAudioProcessorEditor` fallback |

**All three are YOURS to commit, in your one plugin commit.** They are not
somebody else's work in flight. Read the diff (`git diff -- plugins/<Name>`)
before you commit it — you are signing it.

The processor change exists because the param-dump console target builds with
`JUCE_WEB_BROWSER=0` and does not compile the editor TU, so a top-of-file
`#include "PluginEditor.h"` breaks the link. Nine of the ten M1 plugins needed
it; O-Emulator already had it. Under a normal build `JUCE_WEB_BROWSER=1` and
behaviour is byte-identical.

### Batch M1 — 10 plugins, 72 parameters

| Plugin | Params | Frame | Served root | Page shape | Version → |
|---|---|---|---|---|---|
| O-AnalogSaturation | **4** | 600×450 | `Source/ui/public` | single-file | 1.2.0 → 1.3.0 |
| O-Bass | **5** | 420×320 | `Source/ui/public` | single-file | 1.4.0 → 1.5.0 |
| O-Emulator | **5** | 620×430 | `Source/ui/public` | single-file | 1.1.0 → 1.2.0 |
| O-Comp | **7** | 620×360 | `Source/ui/public` | single-file | 1.6.0 → 1.7.0 |
| O-Tremolo | **7** | 600×400 | `Source/ui/public` | single-file | 1.7.0 → 1.8.0 |
| O-Chorus | **8** | **700×125** | `Source/ui/public` | single-file | 1.3.0 → 1.4.0 |
| O-DigiDelay | **8** | 700×196 | `Source/ui/public` | single-file | 1.3.0 → 1.4.0 |
| O-SimpleReverb | **8** | 500×350 | `Source/ui/public` | single-file | 1.6.0 → 1.7.0 |
| O-Bassoon | **10** | 900×600 | `Resources/ui` | single-file | 1.1.0 → 1.2.0 |
| O-Texture | **10** | 800×600 | `Source/ui/public` | `js/main.js` + `css/` | 0.2.0 → 0.3.0 |

**"single-file" means what it says, and the plan does not describe it.** Nine of
the ten have NO `css/` directory and NO controller `.js` beside `i18n.js` — the
page is one `index.html` with inline `<style>` and inline
`<script type="module">`. Your renderer and your tooltip CSS go INTO
`index.html`. Only O-Texture has the `js/main.js` + `css/` shape the plan
assumes. Verify the shape yourself; never assume it.

**Version bumps are MINOR.** A user-visible feature is arriving. O-Bassoon's
`CMakeLists.txt` spells its version `VERSION "1.1.0"` **with quotes** — keep
them.

### Starting i18n state — measured, per plugin

| Plugin | `I18N` | `LABELS` | `I18N_EXEMPT` | `TIP_BINDINGS` |
|---|---|---|---|---|
| O-AnalogSaturation | 0 | 9 | 10 | 0 |
| O-Bass | 0 | 19 | 4 | 0 |
| O-Emulator | 0 | 15 | 16 | 0 |
| **O-Comp** | **3** | 22 | 4 | 0 |
| O-Tremolo | 0 | 21 | 11 | 0 |
| O-Chorus | 0 | 18 | 4 | 0 |
| O-DigiDelay | 0 | 22 | 5 | 0 |
| O-SimpleReverb | 0 | 19 | 12 | 0 |
| O-Bassoon | 0 | 32 | 4 | 0 |
| O-Texture | 0 | 15 | 12 | 0 |

**O-Comp's three `I18N` entries are `canvas.envelope`, `canvas.gainReduction`
and `canvas.gr` — canvas `fillText` strings with EMPTY bodies, per the K4
decision. They are NOT tooltips. Do not give them bodies, do not bind them, do
not delete them.** The same rule holds repo-wide for the other empty-body
entries (O-MicrotonalSampler 51, O-Formant 5) when their batches come.

---

## THE GATE FLIPS THE MOMENT YOU AUTHOR THE FIRST BODY

`check-i18n` assertion 2, as it stands today (`scripts/check-i18n.js:529-556`):

```js
check(bindings.length > 0 || tipBodied.length === 0, ...)
```

`tipBodied` is every `I18N` key whose `en` or `fr` carries a **non-empty `b`**.
So while your plugin has no bodies, `TIP_BINDINGS: []` passes and reports
*"0 tip(s) bound — this plugin has no hover-help, which is a state, not a gap"*.

**Your first authored body makes bindings MANDATORY.** An authored body with no
binding fails as *"ORPHANED: N I18N entries carry a body that nothing binds"*.
And every `TIP_BINDINGS[i][1]` must exist in `I18N` (the second half of
assertion 2), so a typo'd key fails as *"dangling"*.

Assertion 1 also tightens on you: **every `I18N` key needs `en` AND `fr`, each
with a string `t` AND a string `b`.** An empty-body entry is legal (`b: ''`);
a *missing* `b` is not.

### T17's verify command does not work, twice over

The plan gives:

```bash
node scripts/check-i18n.js 2>&1 | grep -c 'tips bound: 0'
```

The gate prints `[2] N tip(s) bound`. **`tips bound: 0` never appears**, so that
command reports 0 whatever the truth is — a vacuous verify, the third instance
of `pattern_recorded_gate_command_not_executable_as_spelled` in this task.

The obvious repair is wrong too. `grep -c "0 tip(s) bound"` substring-matches
**`80 tip(s) bound`** and **`70 tip(s) bound`** — O-IntonationPad and
O-MultiBandCompressor — and reports 24 zero-tip plugins where there are 22. The
orchestrator made exactly this mistake while establishing the baseline for this
brief, which is the strongest evidence available that you would too.

**The correct command, and the one to put in your report:**

```bash
node scripts/check-i18n.js 2>&1 | grep -cE '\[2\] 0 tip\(s\) bound'
```

Baseline before M1: **22**. After M1 it must read **12**.

---

## The renderer, specified

Port `setupTooltips` from `plugins/O-simpleFM/Source/ui/public/js/app.js:384-462`.
Read the whole function including its comments — they record why each choice is
what it is — then reproduce its **behaviour** in your plugin's file, styled in
your plugin's own visual system.

Why this family and not O-Tapestop's: it is ~80 lines against ~180, it is
delegated rather than per-element, it follows the cursor and clamps on all four
edges, and it needs no help-toggle state. O-Tapestop's measure-then-pin
placement engine exists to serve a flip-above/below design and three 40 KB clamp
gates; none of the ten M1 pages needs it.

**Non-negotiable properties.** Each is load-bearing and each has a scar behind it:

1. **Delegated on `document`, not `querySelectorAll("[data-tip]")` at setup.**
   No anchor carries `data-tip` until `applyI18n()` has run. A setup-time
   query binds nothing at all and fails silently.
2. **`pointerover` / `pointerout` / `focusin` / `focusout`** — they bubble;
   `pointerenter` / `focus` do not.
3. **`pointerout` ignores a move between two descendants of the SAME anchor**,
   or the tip flickers off and on at every child boundary.
4. **`createElement` + `textContent`, never `innerHTML`.** Localized copy must
   never reach a markup path. `check-i18n` assertion 9 already forbids an angle
   bracket in an `i18n.js` string literal; this is the other half of that rule.
5. **Clamp on all four edges** with an 8 px margin, and place the tip on the
   opposite side of the cursor when it would overflow. In a 700×125 frame
   (O-Chorus) and a 700×196 frame (O-DigiDelay) the clamp is not an edge case,
   it is the normal path.
6. **`pointer-events: none`** on the surface, or the tip steals the hover that
   is keeping it open.
7. **`position: fixed`, `visibility: hidden` until shown.**
   **CORRECTED, and the correction is measured.** The first draft of this brief
   said a visible empty surface "would enter `check-ui-labels`' text sweep and
   every geometry diff on the page." The O-Bass executor negative-controlled it:
   un-hiding the surface left `check-ui-labels` **byte-identically green**,
   because a fixed box at 0,0 has the same rectangle in both languages — it
   neither moves nor changes the visible set. What actually catches an un-hidden
   surface is **`check-i18n` assertion 10** and **`boot-all-uis`'s text count**
   (14 → 16 on O-Bass). Hide it anyway — the reason is that a visible empty
   surface is a visible empty surface — but do not expect `check-ui-labels` to
   tell you.
8. **Escape hides it. `pointerdown` hides it.**

9. **THE FOCUS ARM MUST BE LATCHED TO THE KEYBOARD.** The O-simpleFM reference
   opens a tip on any `focusin`. **A mouse click on a `<button>` focuses it**,
   so the unconditional rule leaves a tip parked on screen after every click.
   Measured on O-Emulator: clicking `#gear-btn` pinned the gear's own tip at
   `[320, 284, 250x115]` directly across the settings popover the click had just
   opened, and it stayed until focus moved. It also made the surface a VISIBLE
   element inside `check-ui-labels`' state sweep — the `[8b]` inert-element count
   went **7 → 9** (the surface plus its `.tip-title` span) purely because the
   state driver clicks.

   **`:focus-visible` is NOT the discriminator.** Chromium reports it false for
   a programmatic `.focus()` following a click, so a gate driving focus directly
   would measure "no tip" and record that as correct — a false pass built into
   the fix. Use an explicit last-input-device latch, cleared by any keydown,
   which is the same rule and is drivable with real events:

   ```js
   let lastInputWasPointer = false;
   document.addEventListener('pointerdown', () => { lastInputWasPointer = true; hide(); });
   document.addEventListener('focusin', (e) => {
     if (lastInputWasPointer) return;
     /* ... open the tip ... */
   });
   document.addEventListener('keydown', (e) => {
     lastInputWasPointer = false;              // the keyboard is driving again
     if (e.key === 'Escape') hide();
   });
   ```

   **Reference: `plugins/O-Emulator/Source/ui/public/index.html:1278-1340`** —
   read the comment as well as the code. If your page has a double-click value
   editor, its `<input>` sits INSIDE an anchor and needs a specific suppression
   (not a general "inputs are exempt" rule — `#lang-select` is a form control
   AND an anchor, and its keyboard tip is the accessibility half of this
   feature).

   **Your render gate must assert BOTH halves separately**: a pointer click
   leaves no tip, AND a real tab-ring walk still opens one. Asserting only the
   first lets the feature silently become "focus never shows a tip".
10. **Call it AFTER `initI18n()`**, inside the same deferred init and the same
   `try/catch`. A top-level call touching a lower `let`/`const` is a TDZ throw
   that kills every later initializer
   (`pattern_module_toplevel_init_tdz`).

**No help on/off toggle in M1.** O-Tapestop and O-Bitrot have one; these ten do
not, and adding one means a fourth control in the gear popover, a persisted
preference through C++, and a `data-tip-always` bypass so the toggle's own tip
still works when tips are off. That is a separate decision, recorded at the foot
of this file. The gear popover in M1 keeps exactly the language selector it has.

---

## Which controls get a tip

**Mandatory:** every parameter in `params.tsv` **that has a control on the
page**, plus `#gear-btn` and `#lang-select`. All ten M1 plugins carry both of
those ids — verified.

**A parameter with no control gets no tip, and that is a FINDING, not a gap.**
O-Bass dumps 5 parameters and exposes 3: `latency_mode`
(`AudioParameterChoice`) and `bypass` (`AudioParameterBool`) have no control in
the WebView in any version, though both are automatable and host-reachable.
Authoring a body you cannot bind makes it an ORPHAN and fails assertion 2, so
the rule as first written was unsatisfiable there. **Do not add a control to
satisfy the count** — that is a feature change with a geometry cost. Report the
parameter, its type, and that it is host-reachable but not page-reachable.

The gear tip is what tells a user hover-help exists at all. Its body must
describe **only what that popover actually contains** — the language selector
and nothing else. Do not copy O-Tapestop's wording, which promises a hover-help
toggle these plugins do not have. A tip that lies is worse than no tip; this
task has already had to rewrite two such sentences (T10 step 2).

**Not in M1:** the preset bar. Those four controls got accessible names from
their deleted `title=` attributes in Stage K and are self-describing. Adding tips
there is optional polish, not scope.

So M1's authored total is **72 parameter tips + 20 chrome tips = 92 entries**,
each with an `en` and an `fr` `{t, b}`.

### The anchor is not always an id

T17 says "bind `TIP_BINDINGS` to the ids the UI already uses." **Verify that on
your page; it is false on at least one.** O-Chorus's eight knobs carry no `id` —
they are `.knob[data-param="rate"]` inside a `.knob-container`. `TIP_BINDINGS`
takes any CSS selector, because `applyI18n` calls `document.querySelector(sel)`,
so `['.knob[data-param="rate"]', 'tip.rate', '.knob-container']` is the correct
form there.

**Bind the WRAPPER the user aims at, not the 4 px SVG stroke.** The third
element of the triple exists for this: the selector finds an addressable child,
`closest(wrapper)` walks up to the cell that owns the hover area. A tip bound to
`circle.knob-vine` is a tip nobody can open.

Every `applyI18n` warning of the form `i18n: tip target not found: <selector>`
is a binding that resolves to nothing. **`boot-all-uis` is the gate that sees
those** — a console warning there is a real failure, not noise.

---

## Authoring the copy

**Title** = the control's display name, from the dump's `name` column. Where the
page's caption differs from the parameter's name, the caption wins — the user is
reading the page, not the automation lane.

**Body** = what the control does, when to reach for it, and it **ends with the
range and unit**. Three sentences at most. This is a tooltip, not a manual.

The range comes from the dump: `textAtMin`, `textAtMax`, and `label` for the
unit. **`label` is empty far more often than the plan implies.** All 8 of
O-Chorus's parameters have an empty `label`; so do all 10 of O-Texture's and all
173 of O-Prism's. O-Comp is the one M1 plugin with real units (`dB`, `:1`, `ms`).

**Check your own dump; the sentence above is a tendency, not a fact about your
plugin.** O-Bass's three exposed parameters each carry a real `label` (`Hz`,
`%`, `dB`), every one agreeing with the page's own formatter — so "O-Comp is the
one M1 plugin with real units" was already false at the second plugin.

Where `label` is empty, **read how the page renders that readout and phrase the
range from the page's own formatter — never invent a unit.** O-Chorus's `rate`
dumps `0.05 .. 5.00` with no label, and its `.knob-value` renders `1.00 Hz`; the
unit is Hz because the formatter says so, not because a chorus rate is usually
in Hz. Cite the formatter's line in your report for every parameter whose unit
you had to recover this way.

A `discrete` / `boolean` parameter's range is its option words, not a number:
O-Comp's `auto_gain` is `Off` / `On`, O-Texture's `SOURCE` is a six-way choice.
Where those option strings are `AudioParameterChoice` options they are **D-01
arm 1 exempt on the page** — but inside a tooltip BODY they are prose describing
the control, and prose is localized. The two rules do not conflict: the *option
in the selector* stays English so the host automation lane agrees; the *sentence
naming it* is French. Say which you did, and why, in the entry's comment.

**D-03 still binds and it binds to NODES, not to sentences.** A readout node
never becomes a `[data-i18n]` element and never gets localized. A number inside
a localized tooltip body is ordinary prose — `−24 to +24 dB` becomes
`−24 à +24 dB` — exactly as the 21 already-shipped tooltip plugins do it.

**French is drafted, `reviewed: false`, every entry.** No exceptions, no
`sameAsEn` unless the string genuinely is identical and you say why in a comment
— that keeps it in the native-speaker worklist instead of hiding it in
`I18N_EXEMPT` forever.

---

## The nine steps, per plugin

1. **Read `plugins/<Name>/.planning/params.tsv`** and the diff already sitting in
   your working tree (`git diff -- plugins/<Name>`). Reconcile the dump's rows
   against the controls actually on the page. **Report any parameter with no
   control and any control with no parameter** — both are real findings.
2. **Read your plugin's Stage K `i18n.js` header comment in full.** It records
   the measured cliffs and every geometry pin on your page. It is the only
   document that tells you which captions are 1.89 px from a wrap.
3. **`grep -rn 'setVisible' plugins/<Name>/Source/`** — abort and report if it
   targets the web view. A hidden `WebBrowserComponent` drops native-function
   completions.
4. **Author `I18N`** — one entry per parameter plus the two chrome entries.
   English first, French second, `reviewed: false` throughout. Keep the existing
   `LABELS` and `I18N_EXEMPT` blocks untouched unless you have a reason, and
   state the reason.
5. **Bind `TIP_BINDINGS`** to the anchors the page actually has, wrapper form
   where the id'd node is not the hover target.
6. **Port the renderer** and its CSS into your plugin's own visual system, per
   "The renderer, specified".
7. **Write `plugins/<Name>/tests/ui_tip_render_check.js`** — the evidence seat.
   See below. Commit it.
8. **Both gates, then every gate in `plugins/<Name>/tests/`, then the build**
   (under the mutex), then `auval`.
9. **Version, CHANGELOG, staging and commit discipline as T10 step 10**, then
   `git show --stat`. **Do not touch `PLUGINS.md`** — the orchestrator owns it.
   Report the row you would have written.

---

## THE EVIDENCE SEAT: `tests/ui_tip_render_check.js`

**No gate in this repo can see a rendered tooltip.** `check-i18n` reads the
table statically. `check-ui-labels` has no tooltip awareness whatsoever.
`boot-all-uis` counts `aria-label` and `title` and never `data-tip`. The three
committed `ui_tooltip_clamp_check.js` gates belong to O-Tapestop, O-Bitrot and
O-ReverseDelay and are built around the *other* renderer family.

So you write the gate that sees it. **Do not port the 40 KB clamp gates** — they
assume measure-then-pin placement with an above/below flip. Write a compact one,
against the stub, at your plugin's **shipping viewport**, and make it assert:

1. **Every `TIP_BINDINGS` selector resolves.** A binding that finds no element
   is a FAIL, not a warning.
2. **Hover each anchor → the tip becomes visible and carries non-empty text.**
   This is the vacuity guard, and it is the assertion that catches the failure
   this whole section exists for. A tip that never showed must FAIL.
3. **The rendered title and body are byte-equal to the `en` entry.** Not
   "contains" — equal. A `.tip-title` that silently kept a previous anchor's
   text passes a `contains` check.
4. **The tip rectangle is fully inside the viewport**, all four edges, at every
   anchor. This is the assertion the small frames exist to break.
5. **`window.__setLanguage('fr')`, then repeat 2-4 against the `fr` entries.**
   French runs 15-20% longer, wraps to more lines against a `max-width` cap, and
   grows the tip's height — so a tip that fits in English can overflow the
   bottom in French. Then `en` again and confirm it comes back.
6. **A negative control.** Plant an over-long body, confirm assertion 4 reports
   the overflow, restore **from a namespaced scratchpad copy**. Never
   `git checkout -- <file>` to restore a plant: it wipes your uncommitted fix
   with it, and O-GrainScatter lost a whole edit that way.

Report which states you drove and which you did not.

---

## Gates — all must pass before the commit

```bash
node scripts/check-i18n.js --plugin <Name> --strict-v2
node scripts/check-ui-labels.js --plugin <Name>
node plugins/<Name>/tests/ui_tip_render_check.js
node scripts/boot-all-uis.js            # the only gate that sees a swallowed binding failure
ls plugins/<Name>/tests/                # run every other gate in it too
./scripts/build-and-install.sh <Name>   # UNDER THE MUTEX
```

`--strict-v2` is **accepted and is a no-op** as of `96b5f2eb` — canon v1 is
deleted and strict is the default. Passing it is harmless; do not report it as a
finding.

`boot-all-uis.js` must stay green for **every** plugin, not just yours. Its
per-plugin `title=` column must stay at **0** — repo-wide native `title=` is
currently 0 and your renderer must not reintroduce one.

**Geometry.** Adding a hidden `position: fixed` surface should move nothing. Run
`check-ui-labels` before and after and confirm `moved=0`, and confirm the
tooltip node does not enter the label sweep. If it does, it is not hidden
correctly. Any pin you add must be **reverted alone and confirmed to re-break
the gate** — check `dw`, not only `dx`; `dx` alone has mislabelled a pin as
decoration seven times in this task.

---

## Shared resources — unchanged from the K brief

- **`build/` is behind the mutex** at `/tmp/claude-501/stagek-build.lock`, and
  so is anything touching the AU cache. Release it the moment the build and
  `auval` are done. Everything else runs outside it.
  **You do not need the mutex for the param-dump** — it is already run.
- **`PLUGINS.md` is the orchestrator's.** Report your row; do not write it.
- **`scripts/` is the orchestrator's.** Found a gate defect? **Stop and report
  it** with the wrong assumption's shape and a negative control. Do not work
  around it — a workaround in one plugin is what hides a repo-wide gate hole.
  Ten gate fixes have landed in this task; an eleventh is likely.
- **The scratchpad is shared. Write everything into `scratchpad/<yourplugin>/`.**
  A bare `measure.js` or `before-en.json` at the root is not yours. The danger is
  not a crash, it is a silent wrong number.
- **`git add` the exact new paths, then `git commit -- <exact file list>`, then
  `git show --stat`.** A pathspec commit takes only TRACKED files, and this
  stage adds new ones (`tests/ui_tip_render_check.js`, `.planning/params.tsv`).
  `HEAD~1` is not your commit's parent in a shared checkout — use `<sha>^`.
- **Do not tag. Do not push. Do not run `screencapture`, for any reason.**

Commit message shape:

```
improve: <Name> vX.Y.Z - hover-help, in both languages (Stage M batch M1)
```

followed by what was found, what diverged from this brief, what was deliberately
not fixed and why, and the tip-render and geometry results.

---

## What to report back

A compact report, not a transcript:

- version shipped + commit sha + `git show --stat` file count
- parameters: dump count vs controls on the page, and any mismatch
- tips authored: parameter / chrome / total, and the `TIP_BINDINGS` count
- every parameter whose unit had to be recovered from the page's formatter,
  with the formatter's file:line
- tip-render gate: anchors driven, states driven, states NOT driven, the
  negative control's result
- geometry: moved-before / moved-after, every pin with its negative control
- `node scripts/check-i18n.js 2>&1 | grep -cE '\[2\] 0 tip\(s\) bound'`
- every divergence from this brief's structural claims
- anything found and deliberately NOT fixed, with the reason
- what is NOT verified

---

## DECISION ITEMS — for the developer, not for you

Record these; do not act on them.

1. **No hover-help on/off toggle.** M1 ships tips that are always on. Two
   shipped plugins (O-Tapestop, O-Bitrot) have a toggle; the other 19 tooltip
   plugins do not. If the suite should be uniform, that is a separate pass
   across 41 plugins, not a Stage M side effect.
2. **The preset bar gets no tips in M1.**
3. **Checkpoint 5 is still outstanding on all 43** — no human has seen any
   French UI, and all 3202 French entries repo-wide are machine drafts.
   Stage M adds roughly 184 more to that worklist in M1 alone.
4. **`<html lang>` still does not follow the language selector.** Canon-owned,
   all 43. Untouched by this stage.


---

# CORRECTIONS AND CARRIED TRAPS FROM THE M1 PILOTS

Appended as they land. Where this section disagrees with anything above, THIS
section wins.

## From O-Bass (v1.5.0, `a983fddd`)

1. **A dumped parameter is not necessarily a control.** See "Which controls get
   a tip" above, now corrected. Reconcile the dump against the page before you
   author anything.

2. **`page.evaluate` with a function-source STRING is a false pass.** Passing
   the *source text* of a function returns the function object, which is
   unserialisable, so the call resolves to `undefined` — and `undefined !== null`
   sails through a truthiness assertion over a surface nobody ever read. The
   gate's first run passed this way. Pass a real function, and assert on a
   value whose shape you can name.

3. **One flip is not enough on a narrow frame.** At 420x320 every anchor in both
   languages placed by flipping to the opposite side of the cursor, two of them
   also hit the 8 px floor, and **the flipped result needs clamping again**. If
   your clamp runs before the flip, or only once, it is wrong on the small
   frames.

4. **"Single-file page" means the markup, the style and the controller are all
   in `index.html`.** There is still a `js/` directory holding `i18n.js` and the
   JUCE bridge, and a `modules/` directory holding the vendored preset manager.
   Do not read the phrase as "this plugin has no `js/`".

5. **The dev manufacturer code is `OuDv`, not `Ouar`** — for `auval -v <type>
   <subtype> OuDv`.

6. **No pin added means no negative control is owed.** Say that explicitly
   rather than leaving the geometry section empty; an empty section reads as
   "not measured".


## From O-Emulator (v1.2.0, `5cf6bba1`)

1. **The renderer defect above.** It is now property 9 of "The renderer,
   specified" and it is MANDATORY. O-Bass shipped without it and was fixed in a
   follow-up; do not repeat that.

2. **The tell was a number nobody had to chase.** `check-ui-labels` PASSED with
   the defect present. What exposed it was the `[8b]` inert-element count moving
   7 → 9 inside a passing run. **Read the counts in a green gate, not only its
   verdict.**

3. **A negative control can report the right verdict for the wrong reason.**
   The first draft of O-Emulator's focus control called `.focus()` on an element
   that was ALREADY focused, which fires no event at all — so "no tip appeared"
   was true and meaningless. It now walks the real tab ring (12 presses to reach
   the gear). If your control cannot fail, it is not a control.

4. **"Bind to the ids the UI already uses" is false again** — 5 of O-Emulator's
   7 anchors are CSS selectors, not ids. That is now three plugins out of three.

5. **Untracked `.planning/i18n-{index-draft.html,inventory.tsv,labels-skeleton.js}`
   exist for every plugin in the repo.** They are `i18n-extract.js` scratch, they
   are not yours, and they are not `params.tsv`. **Leave them uncommitted.**

## From the orchestrator, applying the latch to the two pilots that landed without it

1. **BLUR BEFORE THE CLICK, or your focus-latch assertion is DECORATION.**
   This is the single most likely way to get this stage wrong, and the
   orchestrator did get it wrong first — while writing the control for exactly
   the trap O-Emulator had just reported.

   An earlier section of a render gate leaves focus on `#gear-btn` (it is the
   last thing the popover-open state touched). **Clicking an already-focused
   element fires no `focusin` at all**, so "no tip after a click" is true for a
   page with no latch whatsoever. The first version of the assertion passed
   **125/125 with the latch deleted**.

   ```js
   await page.evaluate(() => document.activeElement && document.activeElement.blur());
   await page.waitForTimeout(100);
   await page.click('#gear-btn');
   ```

   With the blur, the control fires: **5110 px²** on O-Bass, **4669 px²** on
   O-AnalogSaturation — and the keyboard half stayed green in all four runs,
   which is what proves the two assertions are independent rather than one
   assertion counted twice.

   **Run your negative control BOTH ways.** A control that passes with the fix
   removed is not a control; it is a second copy of the claim.

2. **Measure the overlap, do not just observe the tip.** Intersecting the tip
   rect with the popover rect turns "a tip is showing" into a number that a
   later run can compare. `146 x 35` and `161 x 29` are what made this defect
   reportable rather than arguable.

3. **A green gate's COUNTS are evidence; its verdict is not.** `check-ui-labels`
   passed on both plugins with the defect present, because it classes the
   surface as `pointer-events: none` decoration. The count that moved was
   `[8b]`, inside a passing run.

4. **An already-open tip does not re-render on a language change** — canon
   behaviour, shared by all 21 shipped tooltip plugins. Found by the
   O-AnalogSaturation executor, with a consequence worth carrying: on that page
   `check-ui-labels` assertion 7 is green **partly because of it**. The gate's
   state driver clicks `#gear-btn`, which opens the gear's tip, so the surface
   enters the non-label element set (26 → 28) — and its rect is identical in
   `en` and `fr` **only because it never re-rendered**. If the canon is ever
   taught to refresh an open tip, assertion 7 begins comparing tip rectangles
   across languages on every Stage M plugin, and French wraps taller. Do not
   "fix" this; it is canon-owned and it is a decision item.

## From O-Comp (v1.7.0, `40a156b4`) — the trap whose obvious fix is deleting the fix

1. **A STATIC regex for `lastInputWasPointer` stays green with the latch's guard
   clause removed.** The declaration, the `pointerdown` write and the `keydown`
   clear all survive; only `if (lastInputWasPointer) return;` is gone, and every
   grep still matches. **Only the behavioural control discriminates.** If your
   gate checks for the latch by searching the source, it is checking that
   somebody typed a variable name.

2. **A keyboard-tab probe that samples DURING the fade reports a false "never
   opens" — and the obvious response to that failure is to delete the latch.**
   O-Comp's first tab-ring control slept 80 ms into a 120 ms opacity transition
   while treating anything below opacity 0.99 as hidden, and reported "none in
   20 tabs" for a path that demonstrably works (tab #5). An executor who trusts
   that reading concludes the latch broke the keyboard half and removes it,
   which makes the click defect green again by way of a probe artefact.

   **Await the transition, or assert on `visibility` rather than a hard opacity
   threshold.** And when a keyboard control fails, verify the failure by hand
   before touching the renderer — this is
   `pattern_quiesce_before_stimulus_in_async_ui_gates` with a fix-deleting
   consequence attached.

3. **Bind the chrome BARE, not through a wrapper**, wherever the gear and the
   language selector share an ancestor. On O-Comp `.settings-cluster` contains
   both the gear and the popover, so a wrapper walk makes hovering `#lang-select`
   resolve to the gear's tip.

4. **"The anchor is not always an id" was FALSE on O-Comp** — every knob
   container and the toggle carry ids. The *wrapper* half is still load-bearing
   there: `#threshold-knob` is only the 52 px vine face, so the binding walks
   `closest('.control-group')`. **Four plugins out of four**, the naive reading
   of T17 ships a tip nobody can hold open — but the reason differs per plugin.
   Check both halves separately.

5. **SETTLED, 2026-08-30 — the French decimal separator is a COMMA.** M1 split
   on this: O-Comp and O-SimpleReverb shipped the readout's POINT
   (`0.1 à 100 ms`), reasoning that `.value-display` formats with a point in
   both languages under D-03; O-Chorus shipped the COMMA, matching all 21
   already-shipped tooltip plugins. **The developer chose the comma.** Both
   plugins were corrected in `f0eb50c8`.

   **The rule, for every remaining plugin:** a tooltip body is PROSE and takes
   French convention — decimal comma, a space before `%`, U+2212 for the minus.
   The READOUT keeps its point, because D-03 exempts the readout NODE and that
   has not moved. They differ on purpose: the readout is a machine-formatted
   value, the body is prose.

   Scan your own French entries for `\d+\.\d+` before you commit. O-SimpleReverb
   shipped a point without flagging it at all, so the split was wider than the
   one plugin that reported it.

## From O-Tremolo (v1.8.0, `85e94b5f`) — control the control, and size the plant to the frame

1. **THE BLUR IS LOAD-BEARING, and O-Tremolo is the one that proved it.** With
   the latch removed AND `activeElement.blur()` deleted from the gate, the suite
   passed **186/186**. That is a control ON the control: it demonstrates that
   the blur line is what gives the focus assertion its power, rather than being
   defensive tidying. Run it. If deleting the blur does not turn your failing
   run green, your gate is not shaped the way you think it is.

2. **A PLANT SIZED BY HABIT IS A CONTROL THAT CANNOT FAIL.** A 40x plant — 880
   chars, roughly 390 px tall — **fit and reported nothing** on a 400 px frame
   with 384 px of clamp room. The clamp is doing its job; the plant was simply
   too small to defeat it. **Size the plant against your own frame and your own
   `max-width`**, then confirm it actually overflows before you call the control
   passed. A plant that fits is indistinguishable from a gate that cannot see.

3. **A wrapper class can appear TWICE.** `.waveform-section` matches two nodes
   on O-Tremolo's page, the second wrapping the canvas. `closest()` from the
   `<select>` reaches the correct one; a bare `querySelector` on the class would
   have been right **by luck**. Prefer the `[selector, key, wrapper]` triple's
   `closest()` walk over a class query, and check whether your wrapper class is
   unique before trusting either.

4. **"Bind to the ids the UI already uses" — the score is now four out of four,
   for a DIFFERENT reason each time.** O-Chorus: no ids at all. O-Emulator: 5 of
   7 anchors are CSS selectors. O-Comp: every selector is an id but every target
   is a wrapper. O-Tremolo: every selector is an id and 4 of 8 targets are
   wrappers. **The selector half and the target half fail independently. Check
   both.**

5. **A correction to the orchestrator's own dispatch.** The O-Tremolo dispatch
   said this plugin's Stage-K English-in-a-French-UI fix was recorded in its
   `i18n.js` header comment. **It is not** — the fix is real and shipped at
   v1.7.0 (`addSection()` takes a writer callback so the `'Factory'`/`'User'`
   literals stay at the call site where assertion 13 can read them), but it is
   recorded in commit `af3610dd` and in an `index.html` comment at the call site.
   Read the commit, not only the header.

---

# BATCH M2 — the eight cheapest of the remaining twelve

Everything above still binds. This section carries M2's own measured numbers and
the three places M2 diverges structurally from M1. Where it disagrees with the
M1 tables above, THIS section wins for an M2 plugin.

## Baseline, measured this session

```
node scripts/check-i18n.js 2>&1 | grep -cE '\[2\] 0 tip\(s\) bound'   →  12
node scripts/check-i18n.js                                            →  ALL CHECKS PASS, 43 plugins
```

The twelve are O-AnalogEQ, O-Bells, O-Bowed, O-Detune, O-Formant, O-Freeze,
O-GrainScatter, O-MicrotonalSampler, O-Prism, O-Reed, O-TextureForge, O-Wind.
**M2 takes eight of them. After M2 the count must read 4** — O-Bells, O-Formant,
O-Prism and O-Wind are M3.

## THE PARAM DUMPS ARE WIRED AND RUN — by the orchestrator, this session

Same contract as M1. In your plugin's working tree, **uncommitted**, and
**yours to commit in your one plugin commit**:

| Path | What it is |
|---|---|
| `plugins/<Name>/.planning/params.tsv` | your authoritative parameter inventory |
| `plugins/<Name>/CMakeLists.txt` | the `option(OUARICON_BUILD_TESTS ...)` + `ouaricon_add_param_dump()` block |
| `plugins/<Name>/Source/PluginProcessor.cpp` | the `#if JUCE_WEB_BROWSER` include guard + `GenericAudioProcessorEditor` fallback |

Read the diff (`git diff -- plugins/<Name>`) before you commit it — you are
signing it. **Both arms are already compile-verified**: the isolated configure
built each plugin's own shared-code target (`JUCE_WEB_BROWSER=1`, editor TU
compiled) *and* its `-param-dump` target (`JUCE_WEB_BROWSER=0`, editor TU
excluded), 0 failures across all eight.

The untracked `.planning/i18n-{index-draft.html,inventory.tsv,labels-skeleton.js}`
beside `params.tsv` are `i18n-extract.js` scratch and are **not yours** — leave
them uncommitted (carried from O-Emulator).

### Divergence 1 — O-Bowed already declared `OUARICON_BUILD_TESTS`

Its render harness owns the option. The param-dump `include(...)` +
`ouaricon_add_param_dump()` were added **inside the existing block**, not as a
second `option()` declaration. Seven plugins got the appended block; O-Bowed got
an extended one.

### Divergence 2 — O-TextureForge needed six guard sites, not one

The M1 processor change was a two-line move. O-TextureForge's processor also
calls the editor from four `dynamic_cast<TextureForgeEditor*>(getActiveEditor())`
sites inside `setStateInformation()` and `loadCorpusFile()`, so the include stays
at the top **behind `#if JUCE_WEB_BROWSER`** and each of the four call sites plus
`createEditor()` carries the same guard (`+27 / -0`). Every guarded arm is the
original code verbatim; the `#else` arms are `juce::ignoreUnused`. Under a normal
build `JUCE_WEB_BROWSER=1` and behaviour is byte-identical to v1.1.0.

### FINDING, not fixed — a fresh configure of this repo cannot build O-TextureForge

`CMakeLists.txt:5` caches `CMAKE_OSX_DEPLOYMENT_TARGET` at **10.13**. At that
target O-TextureForge fails to compile: `_deps/knncolle-src` needs
`std::filesystem`, which is `unavailable: introduced in macOS 10.15` — 20+ errors
in `Prebuilt.hpp`, `utils.hpp` and `distances.hpp` before `-ferror-limit` stops
it. The committed `build/` only works because its cache says **11.0**, set at
some point in the past and never written down.

The M2 param-dump configure was rerun with `-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0`
to match `build/`, and everything then built. **Nothing in the repo was changed
for this.** It is a latent fresh-clone / CI hazard for one plugin and it belongs
to whoever owns the root `CMakeLists.txt`, not to Stage M. Recorded as a decision
item.

## Batch M2 — 8 plugins, 177 parameters

| Plugin | Params | Frame | Served root | Page shape | Version → |
|---|---|---|---|---|---|
| O-Freeze | **12** | 550×530 | `Source/ui/public` | single-file (3 inline modules) | 2.1.0 → 2.2.0 |
| O-TextureForge | **12** | 900×600 | `Source/ui/public` | **webpack bundle + `js/i18n_init.js`** | 1.1.0 → 1.2.0 |
| O-AnalogEQ | **16** | **920×220** | `Source/ui/public` | single-file (3 inline modules) | 1.2.0 → 1.3.0 |
| O-Detune | **18** | 600×480 | `Source/ui/public` | single-file (3 inline modules) | 1.6.0 → 1.7.0 |
| O-MicrotonalSampler | **19** | 900×640 | `Resources/ui` | `js/sampler-app.js` + `js/tuning-panel.js` + `css/` | 1.24.0 → 1.25.0 |
| O-Bowed | **29** | 900×600 | `Resources/ui` | single-file (2 inline modules) | 1.5.0 → 1.6.0 |
| O-Reed | **35** | 900×600 | `Resources/ui` | single-file (3 inline modules) | 1.2.0 → 1.3.0 |
| O-GrainScatter | **36** | 900×800 | `Source/ui/public` | `js/app.js` + `js/i18n.js`, styles inline | 2.5.0 → 2.6.0 |

Every dump count was produced by the runtime walk and agrees with the plan's
figure for all eight. **Versions are MINOR** — a user-visible feature is arriving.
**O-Bowed and O-Reed spell their version `VERSION "1.5.0"` / `VERSION "1.2.0"`
with quotes — keep them.**

**`O-AnalogEQ is 920×220.`** That is a 220 px tall frame carrying 16 tips. It is
this batch's O-Chorus: the four-edge clamp is the normal path there, not an edge
case, and O-Bass's carried trap 3 — *one flip is not enough, and the flipped
result needs clamping again* — is the assertion most likely to catch you.

### Divergence 3 — O-TextureForge's renderer does NOT go in the bundle

It is the only webpack-bundled page in the suite. `Source/ui/src/app.js` compiles
to `Source/ui/public/js/app.bundle.js`, and the canon deliberately does not live
there: webpack would resolve `import './i18n.js'` at build time and inline the
label table, leaving the served `js/i18n.js` read by nobody. That is why
`js/i18n_init.js` exists — read its header comment, which explains it in full.

**Your renderer and your tooltip CSS belong beside the canon in
`js/i18n_init.js` and the page's own stylesheet — NOT in `src/app.js`.** Putting
them in the bundle source means a webpack rebuild inside a plugin commit, which
is not this stage's scope and produces a 220 KB diff nobody can review.

## Starting i18n state — measured, per plugin

| Plugin | `I18N` | `LABELS` | `I18N_EXEMPT` | `TIP_BINDINGS` |
|---|---|---|---|---|
| O-Freeze | 0 | 17 | 5 | 0 |
| O-TextureForge | 0 | 27 | 14 | 0 |
| O-AnalogEQ | 0 | 17 | 7 | 0 |
| O-Detune | 0 | 29 | 15 | 0 |
| **O-MicrotonalSampler** | **51** | 199 | 19 | 0 |
| O-Bowed | 0 | 41 | 3 | 0 |
| O-Reed | 0 | 55 | 7 | 0 |
| O-GrainScatter | 0 | 49 | 24 | 0 |

**O-MicrotonalSampler's 51 `I18N` entries are toast and JS-written strings with
EMPTY bodies** — verified: zero non-empty `b:` in the whole block. Per the K4
decision they are NOT tooltips. **Do not give them bodies, do not bind them, do
not delete them.** Your authored tips are new keys alongside them, and only YOUR
keys go in `TIP_BINDINGS`.

All eight carry `#gear-btn` and `#lang-select` — verified, one occurrence each.
All eight already have `tests/i18n-states.json`; O-Bowed also has
`tests/render-harness/` and O-MicrotonalSampler `tests/fixtures/` and
`tests/ui-stub/`. **Run every gate you find in `tests/`, not only the new one.**

M2's authored total is **177 parameter tips + 16 chrome tips = 193 entries**,
each with an `en` and an `fr` `{t, b}` — minus whatever your own reconciliation
finds is a dumped parameter with no control on the page. That subtraction is a
FINDING; report it, do not add a control to satisfy the count.

## The French decimal separator is SETTLED — it is the COMMA

Not a live question in M2. A tooltip body is PROSE and takes French convention:
decimal comma, a space before `%`, U+2212 for the minus. The READOUT keeps its
point (D-03 exempts the readout NODE). **Scan your own French entries for
`\d+\.\d+` before you commit** — O-SimpleReverb shipped a point in M1 without
flagging it at all.

## The M1 traps that cost the most, in one list

Read the six correction sections above in full. These are the ones an M2 executor
is most likely to reproduce:

1. **The focus latch is MANDATORY** (renderer property 9). A click focuses a
   `<button>` and parks a tip on screen. `:focus-visible` is not the
   discriminator.
2. **`activeElement.blur()` before the click, or your focus assertion is
   decoration.** O-Tremolo proved it: latch removed AND blur removed → 186/186
   green. Run your negative control BOTH ways.
3. **A static regex for `lastInputWasPointer` stays green with the guard clause
   deleted.** Only a behavioural control discriminates.
4. **A keyboard-tab probe sampling mid-fade reports a false "never opens", and
   the obvious response is to delete the latch.** Await the transition or assert
   on `visibility`.
5. **Size the plant against YOUR frame.** A 40× plant fit inside a 400 px frame
   and reported nothing. A plant that fits is indistinguishable from a gate that
   cannot see. On a 220 px frame (O-AnalogEQ) the arithmetic is different again.
6. **"Bind to the ids the UI already uses" was wrong on five plugins out of
   five, for a different reason each time.** The selector half and the target
   half fail independently. Check both.
7. **Bind the chrome BARE where the gear and the selector share an ancestor**,
   or hovering `#lang-select` resolves to the gear's tip.
8. **Read the COUNTS in a green gate, not its verdict.** `check-ui-labels`
   passed on two plugins with the focus defect present; the `[8b]` inert-element
   count is what moved.

## Batch M2 additions to the decision items

5. **The root `CMAKE_OSX_DEPLOYMENT_TARGET` is 10.13 and O-TextureForge cannot
   build at it.** See the finding above. Latent fresh-clone / CI hazard, one
   plugin, root-CMakeLists-owned.

---

# BATCH M2 COMPLETE — 8 of 8, and what M3 inherits

All eight landed 2026-08-30 22:35–22:45. **The zero-tip count went 12 → 4**, and the
four are exactly O-Bells, O-Formant, O-Prism, O-Wind — the M3 set. Repo-wide
`check-i18n` ALL CHECKS PASS across 43; `boot-all-uis` 43/43 clean, 0 warn,
0 failed, native `title=` still 0 repo-wide.

| Plugin | Version | Commit | Files | Params → controls | Tips | Render gate |
|---|---|---|---|---|---|---|
| O-Detune | 1.7.0 | `a4c243af` | 7 | 18 → 16 | 18 | 393/393 |
| O-Freeze | 2.2.0 | `4090e6ac` | 7 | 12 → 12 | 14 | 311/311 |
| O-TextureForge | 1.2.0 | `56be79ca` | 9 | 12 → 12 | 14 | 287/287 |
| O-AnalogEQ | 1.3.0 | `308b3360` | 7 | 16 → 15 | 13 | 308/308 |
| O-Bowed | 1.6.0 | `eb62babf` | 7 | 29 → 28 | 30 | 457/457 |
| O-Reed | 1.3.0 | `fadd9b7e` | 7 | 35 → 33 | 35 | 523/523 |
| O-MicrotonalSampler | 1.25.0 | `adb7fc4f` (+`9a29b026`) | 10 | 19 → 18 | 21 | 349/349 |
| O-GrainScatter | 2.6.0 | `5398b5a2` | 8 | 36 → 36 | 38 | 796/796 |

**183 tips authored** (166 parameter + 16 chrome + 1 double-binding), every French
entry `reviewed: false`. **3424 render-gate assertions**, 0 failures. Zero decimal
points in any French body across all eight — the comma held.

## SEVEN parameters are host-reachable and page-unreachable

M1 found one (O-Bass). M2 found seven across five plugins, and **not one is
vestigial** — each is automatable, each reaches the DSP:

| Plugin | Parameter | Why there is no control |
|---|---|---|
| O-Detune | `focus_low`, `focus_high` | DSP writes their coefficients every block (`PluginProcessor.cpp:602-603`), relayed to the WebView, page even builds slider states and formatters. A DAW can automate a band-limit the user cannot see. |
| O-AnalogEQ | `output_gain` | Removed in the v1.0.5 UI simplification, documented at `PluginProcessor.cpp:88`, kept because factory presets set it. |
| O-Bowed | `tuningSystem` | `PluginEditor.cpp:78` builds a `WebComboBoxRelay`; the page's `bindComboBox()` helper (`index.html:1275`) is never called. |
| O-Reed | `referencePitch` | Its control is in the **shared** `scala-tuning-engine` panel, lazy-mounted on first Tuning-tab click, absent from the DOM when `applyI18n()` runs. |
| O-Reed | `tuningSystem` | `bindComboBox('tuningSystem','tuningSystem')` resolves to `null` and warns on every load. |
| O-MicrotonalSampler | `rr_mode` | No control in any version; no `setRrMode` native fn exists. |

**The rule held: none of the eight added a control to satisfy the count.** Three
plugins — O-Freeze, O-TextureForge, O-GrainScatter — matched exactly, in both
directions.

---

# CORRECTIONS AND CARRIED TRAPS FROM M2

Where this section disagrees with anything above, THIS section wins.

## 1. THE POST-FLIP RE-CLAMP IS UNREACHABLE BY CONSTRUCTION — on all eleven ports

From O-AnalogEQ, and it reaches back across every M1 plugin. In `position()` the
re-clamp reads `ny = innerHeight - M - r.height`, then `Math.max(M, ny)`. **After
a flip `ny = y - h - 12`, so the test `ny + r.height > innerHeight - M` collapses
to `y - 12 > innerHeight - M` — it stops mentioning the tip's size at all**, and
can only fire for a cursor outside the viewport. Same collapse on x
(`nx + r.width` → `x - 14`).

O-Chorus's copy credits that line with *"every vertical placement on this page
lands on the line below, not on the flip above it"*. **The behaviour is real; the
credited line is not producing it.** The `Math.max(M, ny)` floor is. Deleting the
floor puts a tip at `top −95.1` — 103 px off the page — while all 13 shipped tips
stay green.

**M3: do not delete the line, and do not claim it.** Drive the floor directly with
its own assertion, the way O-AnalogEQ's `[4c]` does. This is
`pattern_review_recomputes_instead_of_measuring` wearing a clamp.

## 2. A TENTH RENDERER PROPERTY — the drag guard — and it is CONDITIONAL

From O-TextureForge. Knobs that start a drag on `mousedown` and track
`document.mousemove` will open a NEIGHBOUR's tip over the control being turned
when the drag strays into another cell. **`pointerdown` alone cannot cover it —
`pointerover` arrives after it.** The fix is a `pointerHeld` flag cleared on
`pointerup`/`pointercancel`, and the control must assert the RELEASE as well as
the hold, or it is a permanent off switch that passes.

**But O-AnalogEQ needed none**: `setPointerCapture(e.pointerId)` retargets every
boundary event for the duration of a drag, so the guard is already there in the
page's own code. **Check which shape your page has before adding a flag** — and
drive the drag either way, because "no guard needed" is a measurement, not a
default.

## 3. `applyI18n` FALLS BACK `el.closest(w) || el` — so the hover sweep is BLIND to a broken wrapper

From O-Reed, and it is the most important gate-shape finding of the batch. Break
one binding's wrapper and:

- assertion **[1] fails** (the selector resolves, the wrapper does not)
- assertions **[2], [3], [4] all still PASS**, in both languages

Because the fallback hands back the bare element, the tip still opens — on the
wrong-sized cell. **A gate that checks only "the tip appeared with the right text
inside the viewport" cannot see a broken wrapper at all.** That is why [1] must be
a hard FAIL and never a warning. Same family as finding 1: an assertion that looks
like it covers something it cannot see.

## 4. AN ANCHOR CAN BE UNOPENABLE WHILE PASSING EVERY STATIC CHECK

Three distinct shapes, three plugins:

- **O-MicrotonalSampler** — `#ctrl-attack` is the **1×1 px, opacity-0,
  `pointer-events: none`** `<input type="range">` inside the cell. It is an id,
  it resolves, `check-i18n` is happy, and no human could ever open its tip. The
  addressable node is `[data-knob-id="ctrl-attack"]`.
- **O-AnalogEQ** — `.knob-outer` and `.knob-inner` are **both**
  `pointer-events: none` (`index.html:229, :271`). The only node receiving a
  pointer event is their parent, which resolves outer-vs-inner from the cursor's
  **distance from the centre** (`INNER_THRESHOLD 0.60`), not a child boundary. So
  **two parameters share one hover target and one tip naming both rings.**
- **O-GrainScatter** — 14 of 36 controls are `pointer-events: none` **in the
  DEFAULT state**, from two feature gates (`setupPitchGate` `app.js:326-345`,
  `setupSpatialGate` `:351-375`).

**Assert a minimum hover area, and assert that every binding lands on a DISTINCT
node.** Four of the eight added the distinct-node assertion independently; make it
standard. A second row silently overwriting the first passes `check-i18n` while
reporting two bound tips.

## 5. DRIVE OUT OF A HIDDEN STATE THROUGH THE PAGE'S OWN PATH, NEVER BY STRIPPING THE CLASS

O-GrainScatter's 14 gated controls, O-Bowed's `#sympAmount-ctrl` (`display: none`
at `sympatheticCount`'s default of 0), O-Detune's Width slider (`.disabled` →
`pointer-events: none` while `mono_safe` is on). All are default states.

Every one of the three drove the parameter through the stub's own
`valueChangedEvent` handler rather than removing the class. **Stripping the class
tests a state the plugin never reaches.** O-GrainScatter's gate then pins the rest
state as its own assertion, so the gating cannot silently disappear either.

Note also: **`pointer-events: none` does NOT remove an element from the tab ring.**
O-GrainScatter's three gated `<select>`s open their tip from the keyboard while
unreachable by mouse. Half-reachable is a state; report it rather than calling it
dead.

## 6. THE BLUR IS LOAD-BEARING ON SEVEN OF EIGHT — and the eighth is the instructive one

O-TextureForge ran the full 2×2 and found the blur made **no difference**: latch
removed fails at 4648 px² with or without it, because the section preceding that
control is pure mouse work and the French sweep had already blurred the gear.

**It kept the blur and recorded in the gate that this is an accident of section
order, one edit from decoration.** That is the correct handling. O-Tremolo's
finding is about a CLASS of gate, not a universal law — **run the 2×2 and report
which cell you are in**, rather than asserting the blur matters because a previous
plugin said so.

The static-grep half is now confirmed on six plugins: **deleting only
`if (lastInputWasPointer) return;` leaves every `grep -c lastInputWasPointer`
green.** Label that grep a presence note in your gate, never the control.

## 7. THE EXECUTABLE GATE MAY NOT LIVE IN `tests/`

O-MicrotonalSampler's `tests/` holds only DATA — `i18n-states.json`, `fixtures/`,
`ui-stub/`. The runnable gate is **`Source/tests/ui_frontend_check.js`** (20/20).
The brief's "run every gate in `tests/`" would have missed it. **Look in
`Source/tests/` too**, and in `tests/render-harness/`.

**And run the render harness where one exists.** O-Bowed's canonical preset
rendered **bit-identical** to the committed golden
(`93124fb8dd8223caafac5948c988a226230363d79a17323d386e9a1db34c8891`) — which is
the only MEASURED evidence in this stage that the `PluginProcessor.cpp` include-
guard change is DSP-neutral. Everywhere else it is argued.

## 8. "BIND TO THE IDS THE UI ALREADY USES" — one plugin in eight, and it is not the rule

M2's score, with the reason each time:

| Plugin | Selector half | Target half |
|---|---|---|
| **O-Freeze** | **TRUE** | **TRUE** — `.knob` is a flex COLUMN holding visual + caption + readout, so the id already IS the cell. Every binding bare, no `closest()` anywhere. |
| O-Detune | TRUE | false — 11 of 18 walk to a wrapper |
| O-Bowed | false (28 of 30 have no id) | not needed — `.knob-control` IS the 62 px cell |
| O-Reed | false for 27 of 35 | true for those 27, false for the 5 `<select>`s |
| O-AnalogEQ | false — the naive ids are `pointer-events: none` | false — the walk widens the cell 4225 → 7225 px², +71% |
| O-TextureForge | false — 11 of 14 have no id | false — caption and readout are SIBLINGS of the 44 px circle |
| O-MicrotonalSampler | false — the id is a 1×1 px hidden input | false — and two wrapper classes match TWICE |
| O-GrainScatter | false — **3 of 38**, the page has 4 ids total | false — the same 34 need `closest()` |

**O-Freeze is the only plugin in fourteen where the naive reading of T17 is
correct on both halves.** Check the two halves separately, every time.

**Chrome binds BARE on all eight** — `.settings-cluster` (or its equivalent) holds
the gear and the popover on every page in this batch. O-TextureForge found the
mirror image as well: `#midi-mode` shares `.bottom-controls` with `#drop-zone`, so
a wrapper would put the MIDI tip over the drop zone.

## 9. "THE CAPTION WINS" REVERSES WHEN THE CAPTION IS A TRUNCATION

O-Reed (27 captions cut to a 68 px box, three ending in a truncating period),
O-MicrotonalSampler (`Poly`, `Vel-XF`, `Expr`, `Dyn Rng`, `Out Gain` at a 56 px
column) and O-Bowed all shipped the parameter's FULL name as the tip title.

**The rule is for a caption that DISAGREES with the parameter name. A caption that
is the same name with letters missing is not a disagreement** — and a 260 px
tooltip is precisely where `Reed Hard.` should become `Reed Hardness`, which is
also the automation-lane name. O-GrainScatter took the other branch and shipped
six abbreviated French titles; it flagged them for the native-speaker review and
opened each body by naming the control in full. Both are defensible; **say which
you chose and why.**

## 10. TWO SHIPPED DEFECTS FOUND WHILE WRITING COPY — this is what the stage is for

Neither is a tooltip bug. Both were found because somebody had to describe what a
control does and looked:

- **O-TextureForge: three readouts have been blank since v1.0.0.** `setupKnob` is
  passed `(n) => ''` for `grainSize`, `grainDensity` and `outputGain`
  (`src/app.js:705, :706, :708`), so the first `updateDisplay()` erases the
  authored fallbacks `50ms`, `8`, `0 dB` and nothing replaces them. Measured in
  the harness. Not fixed — webpack-input edit plus a bundle rebuild.
- **O-Reed: `instrumentPreset` is a dead parameter.** `pInstrumentPreset` is
  fetched at `ReedWindVoice.cpp:50` and never `load()`ed anywhere in `Source/`.
  The dropdown moves the automation lane and nothing else. The 21 factory presets
  carrying those voicings are unreachable from the UI (no preset bar) and from the
  host (`getNumPrograms()` returns 1).
- **O-Reed: reported latency does not follow Oversampling.** `setLatencySamples()`
  is called once in `prepareToPlay()` from the default 2× path
  (`PluginProcessor.cpp:392-394`, its own comment admits it), so 4× leaves the
  host compensating by the 2× figure.

Both plugins put the truth in the tooltip body rather than describing the feature
as intended. **A tip that lies is worse than no tip** — that principle has now
rewritten five sentences in this task.

## 11. TWO MORE PLACES A TIP WOULD HAVE LIED

- **O-Bowed's gear popover opens BELOW** (`top: calc(100% + 8px)`). Copying
  O-Chorus's "above" wording would have shipped a false sentence.
- **O-MicrotonalSampler's `technique_select` is 0..7 in the host and 1..8 on the
  page.** The body says "Slots 1 to 8" because the user reads the page.
  Re-ranging an `AudioParameterInt` is host-visible and was not done.

## 12. RESTORE A PLANT FROM A NAMESPACED COPY — eight for eight

Every executor restored by file copy from `scratchpad/<plugin>/`, and several
re-checksummed afterwards. **Nobody ran `git checkout --`.** The scar that rule
comes from is O-GrainScatter's, and O-GrainScatter is the plugin that kept
`.PRISTINE` copies and verified them.

**Size the plant against YOUR frame, and prove it overflows before you call the
control passed.** O-Detune measured ~448 px of clamp room and noted a habitual 40×
plant would have fit and reported nothing. O-AnalogEQ SEARCHED for a plant size
that fails on both sides of the flip rather than guessing one.

## 13. `--amend` IS UNSAFE IN THIS CHECKOUT

O-MicrotonalSampler needed to fix three stale line citations in its own comment
and **committed a second time rather than amending**, because HEAD is shared with
seven concurrent executors and `--amend` rewrites whichever commit is HEAD at the
instant it runs. Correct call. `9a29b026` is comment-only, and it rebuilt and
re-`auval`'d so the installed binary matches HEAD.

Every executor used `<sha>^` rather than `HEAD~1`, and every one of them was
right to: another commit landed between in five of eight cases.

---

## DECISION ITEMS — added by M2

6. **`sanisizer` is pinned to a MOVING BRANCH.**
   `build/_deps/subpar-src/extern/CMakeLists.txt:4-9` declares it with
   `GIT_TAG master`. Upstream rewrote a commit; the local checkout and
   `origin/master` diverged 1↔1, so every CMake **regenerate** runs
   `git pull --rebase`, replays the local copy onto the upstream rewrite of
   itself, and conflicts in `include/sanisizer/float.hpp`. Any `CMakeLists.txt`
   change triggers a regenerate — which was all eight of us. Resolved in the
   DERIVED directory only (`reset --hard origin/master`, under the mutex), and
   provably neutral: the entire delta between the two SHAs is one added comment
   line. **The fix is to pin that FetchContent to a SHA.** Same dependency chain
   as decision item 5.
7. **O-TextureForge's three blank readouts** (finding 10). Webpack-input edit plus
   a bundle rebuild — deliberately out of a tooltip stage's scope.
8. **O-Reed's dead `instrumentPreset` and its oversampling-blind latency**
   (finding 10). Both host-visible; both need a decision, not a patch.
9. **The unreachable post-flip re-clamp across eleven plugins** (finding 1). It is
   per-plugin page code, not `scripts/`, so fixing it is an eleven-file sweep.
10. **O-AnalogEQ's two Q toggles clip their own `TIGHT` option in ENGLISH.**
    `.three-way-option` is `flex: 1` without `min-width: 0`, so three items sit at
    min-content (109.87 px in a 108 px box) and `overflow: hidden` takes 1.87 px
    off `TIGHT`'s right edge. Carried unchanged from v1.2.0, invisible to both
    gates (those nodes are exempt and never keyed).
11. **Keyboard reach is partial on most of this batch by design.** Knob cells are
    pointer-drag `<div>`s with no `tabindex`, so hover-help's keyboard half
    reaches only the chrome and any natively focusable control — 11 of 38 on
    O-GrainScatter, 3 of 14 on O-TextureForge, 2 of 13 on O-AnalogEQ. Adding
    `tabindex` is a feature change with a focus-ring cost. **This is the largest
    open accessibility question the stage has surfaced.**
12. **Checkpoint 5 is now further out, not closer.** M2 adds **183** machine-drafted
    French entries to a worklist no native speaker has started. Repo total 3471.

## STILL NOT VERIFIED, on all eight

No DAW test — `auval` and headless Chromium only, never a real WKWebView. The
Standalone `.app` is stale (`build-and-install.sh` builds VST3 + AU only).
Checkpoint 4 (host session save/reopen) reasoned, not executed. Windows/WebView2
font metrics remain the standing hardware-blocked deferral, and **tooltips are new
surface for it** — French wraps taller inside a fixed `max-width`, so a wider face
shows there first.

---

# BATCH M3 — the last four, and the batch that is bigger than M1 and M2 together

Everything above still binds. Where this section disagrees with anything above,
THIS section wins for an M3 plugin.

## Baseline, measured this session

```
node scripts/check-i18n.js 2>&1 | grep -cE '\[2\] 0 tip\(s\) bound'   →  4
node scripts/check-i18n.js                                            →  ALL CHECKS PASS, 43 plugins
```

The four are **O-Bells, O-Formant, O-Prism, O-Wind** — exactly the set M2 named.
**After M3 the count must read 0.**

### THE SIZE CLAIM, CORRECTED

The dispatch premise said *"O-Prism alone is 173 parameters — larger than M1 and
M2 combined."* **It is not: M1+M2 is 249.** O-Prism at 173 is 2.4× the whole of
M1 and just under the whole of M2 (177).

**What IS true, and is the number that matters: M3 is 358 parameters across four
plugins — 44% more than M1 and M2 combined, in a fifth as many dispatches.**

| Batch | Plugins | Params |
|---|---|---|
| M1 | 10 | 72 |
| M2 | 8 | 177 |
| **M3** | **4** | **358** |

And the real argument for giving O-Prism its own dispatch is **not the count** —
it is a structural one that no plugin in M1 or M2 had. See "O-Prism is a
different problem" below.

## THE PARAM DUMPS ARE WIRED AND RUN — by the orchestrator, this session

**Correction to the dispatch premise, which said param-dumps were wired for none
of the four: O-Prism's has been wired and COMMITTED since Stage A (`5ee01ed4`),**
and its `params.tsv` (173 rows) was already sitting untracked in the tree from
the calibration run that established the repo-wide 607 figure. O-Prism's
executor commits `params.tsv` **only** — its `CMakeLists.txt` and
`PluginProcessor.cpp` are already correct and must not be touched.

The other three were wired and run this session. In O-Bells, O-Formant and
O-Wind's working trees, **uncommitted, and yours to commit in your one plugin
commit**:

| Path | What it is |
|---|---|
| `plugins/<Name>/.planning/params.tsv` | your authoritative parameter inventory |
| `plugins/<Name>/CMakeLists.txt` | +14 lines: the `option(OUARICON_BUILD_TESTS ...)` + `ouaricon_add_param_dump()` block |
| `plugins/<Name>/Source/PluginProcessor.cpp` | +15 lines: the `#if JUCE_WEB_BROWSER` include guard + `GenericAudioProcessorEditor` fallback |

All three took the **M1 two-line shape**, not O-TextureForge's six-site shape —
none of the three has a `getActiveEditor()` or `dynamic_cast<...Editor*>` site in
its processor (verified: zero across all three). Read the diff
(`git diff -- plugins/<Name>`) before you commit it — you are signing it.

**Both arms are compile-verified**, 0 errors: the isolated configure built each
plugin's own target (`JUCE_WEB_BROWSER=1`, editor TU compiled) *and* its
`-param-dump` target (`JUCE_WEB_BROWSER=0`, editor TU excluded).

### Divergence 1 — the isolated configure SKIPPED the two heavy plugins

`cmake -B /private/tmp/claude-501/m3-paramdump -G Ninja -DOUARICON_BUILD_TESTS=ON
-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DSKIP_PLUGINS="O-TextureForge;O-Texture"`.

The root `CMakeLists.txt:43` exposes a `SKIP_PLUGINS` cache list. Using it means
the umappp / knncolle / **sanisizer** FetchContent chain is never entered, which
sidesteps **both** M2 decision items 5 and 6 (the 10.13 deployment-target failure
and the `GIT_TAG master` rebase conflict) rather than working around them. M2's
orchestrator hit both and had to resolve the sanisizer divergence under the mutex.
**M3 hit neither.** Recorded because it is the cheaper recipe, not because
anything changed in the repo.

## Batch M3 — 4 plugins, 358 parameters

| Plugin | Params | Frame | Served root | Page shape | Version → |
|---|---|---|---|---|---|
| O-Wind | **56** | 900×600 | `Resources/ui` | single-file (2 inline modules) + **shared-module** tuning panel | "1.17.0" → "1.18.0" |
| O-Formant | **64** | 800×600 | `Source/ui/public` | index.html (3 inline modules) + `js/main.js` + `js/tuning-panel.js` | 1.26.0 → 1.27.0 |
| O-Bells | **65** | 800×600 | `Resources/ui` | index.html (2 inline modules) + `js/tuning-panel.js` + `css/` | 4.2.0 → 4.3.0 |
| O-Prism | **173** | **1200×800** | `Source/ui/public` | index.html (2 inline modules) + `js/wavetable-editor.js` + `css/` | 1.21.0 → 1.22.0 |

**Versions are MINOR.** **O-Wind spells its version `VERSION "1.17.0"` WITH
QUOTES — keep them.** The other three are unquoted.

**No plugin in this batch has a tight frame.** M1's O-Chorus (700×125) and M2's
O-AnalogEQ (920×220) are behind you; the smallest frame here is 800×600. The
four-edge clamp is a real assertion, not the normal path — **which means your
negative-control plant has to be much bigger to break it. O-Detune measured
~448 px of clamp room and noted a habitual 40× plant would have fit and reported
nothing; on 800×600 and 1200×800 the arithmetic is worse again. SEARCH for a
plant size that fails, the way O-AnalogEQ did. Do not guess one.**

### Divergence 2 — O-Wind serves the tuning panel from the SHARED MODULE

O-Bells and O-Formant each carry a **vendored** `js/tuning-panel.js` inside their
served root. O-Wind does not: `CMakeLists.txt:97-98` embeds
`${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js` and
`snippets/tuning-panel.css` straight into `O-Wind_UIResources`, and
`PluginEditor.cpp:847` serves them at `/js/tuning-panel.js`.

**So a `find` over O-Wind's served root shows no tuning panel and the page still
imports one.** That is a serving shape, not a 404 — do not report it as a missing
file. O-Prism has no shared tuning panel at all; its tuning tab is bespoke inline.

## Starting i18n state — measured, per plugin

| Plugin | `I18N` | `LABELS` | `I18N_EXEMPT` | `TIP_BINDINGS` |
|---|---|---|---|---|
| O-Wind | 0 | 67 | — | 0 |
| **O-Formant** | **5** | 120 | — | 0 |
| O-Bells | 0 | 122 | — | 0 |
| O-Prism | 0 | 155 | — | 0 |

**O-Formant's five `I18N` entries are `canvas.lyrics`, `canvas.plosive`,
`canvas.fricative`, `canvas.mixed` and `js.savePresetAs` — canvas `fillText`
prose and a `window.prompt` caption, every one `b: ''`.** Verified: zero
non-empty `b:` in the block. Per the K4 decision they are NOT tooltips. **Do not
give them bodies, do not bind them, do not delete them.** Its `i18n.js:39-63`
header already explains why they are there; read it. Your authored tips are new
keys alongside them, and only YOUR keys go in `TIP_BINDINGS`.

All four carry `#gear-btn` and `#lang-select` — verified, one occurrence each.
All four have `tests/i18n-states.json`; O-Bells and O-Prism also have
`tests/ui-stub/`. **None of the four has a runnable gate today** — no
`*_check.js` anywhere, and no `Source/tests/` (M2 finding 7's trap does not fire
here, but check anyway). Your `ui_tip_render_check.js` will be the first
executable gate in all four.

`grep -rn 'setVisible' Source/` targets the web view on **none** of the four.

## THE DEFINING PROPERTY OF M3: every page is TABBED, and controls start HIDDEN

M1 and M2 were mostly flat pages. **All four M3 pages are tab decks**, and a
`.tab-content` that is not `.active` is `display: none` — a zero-size rect that
cannot be hovered and cannot be measured.

| Plugin | Tab panels | Active at load |
|---|---|---|
| O-Wind | 1 | — |
| O-Bells | 3 | 1 |
| O-Formant | 4 (`synth` / `effects` / `lyrics` / `tuning`) | `synth` |
| O-Prism | 5 (`synth` / `mod` / `tuning` / `effects` / `wavetable`) | `synth` |

**M2 finding 5 is the governing rule and it is now the batch's main event: drive
out of a hidden state through the page's own path, NEVER by stripping the class.**
Click the tab. Stripping `display:none` off `#mod-tab` measures a state the
plugin only reaches by a click you did not make, and O-Prism has four such tabs.

**And three of the four lazy-`import()` the shared tuning panel** — O-Bells
(`index.html:2999`), O-Formant (`:1424`), O-Wind (`:1993`). That is **O-Reed's
`referencePitch` trap, batch-wide**: the panel is absent from the DOM when
`applyI18n()` runs, so a `TIP_BINDINGS` selector into it resolves to null and
warns. Report it; do not force-mount the panel to satisfy a count.

## SIXTEEN parameters are host-reachable and page-unreachable, BEFORE O-Prism

M1 found 1. M2 found 7. **M3's three smaller plugins carry 16 between them**,
measured as zero occurrences of the parameter ID anywhere in the served root
**and** zero in the shared tuning panel, against 1–5 C++ files each:

| Plugin | Parameters with no page reference |
|---|---|
| O-Bells (4) | `tuning_masterTune`, `tuning_octaveStretch`, `tuning_pitchBendRange`, `tuning_temperamentPreset` |
| O-Formant (7) | `consonantVOT`, `sourceFilterCoupling`, `tuning_masterTune`, `tuning_tuningMode`, `tuning_octaveStretch`, `tuning_pitchBendRange`, `tuning_temperamentPreset` |
| O-Wind (5) | `attackChiff`, `humanize`, `vibratoOnset`, `inharmonicity`, `referencePitch` |

**This is a measurement, not a verdict, and confirming it is step 1 of your
nine.** In particular the `tuning_*` family is exactly where a lazily-mounted
panel would hide a control from a static scan — but note that **none of them
appears in the shared tuning panel module either**, which is what separates them
from O-Reed's `referencePitch`. Check the panel's own markup and its native-
function names before you call one unreachable, and say which way you resolved it.

**Do not add a control to satisfy the count.** Report the parameter, its type,
and that it is host-reachable but not page-reachable.

## O-PRISM IS A DIFFERENT PROBLEM — and this, not 173, is why it gets its own dispatch

O-Prism's 173 parameters split three ways, and the split is structural:

| Class | Count | Anchor exists when `applyI18n()` runs? |
|---|---|---|
| Statically anchored (incl. 61 JS-expanded knobs) | **107** | **YES** |
| Async mod-matrix rows | **64** | **NO** |
| No page reference at all | **2** | n/a |

**1. The 61 knobs are JS-expanded but SAFE.** `expandKnobMarkup()`
(`index.html:1929`) is a synchronous IIFE that rewrites every
`div.knob-container[data-knob]` placeholder into SVG markup during module
evaluation — long before `initI18n()` at `:4193`. Its comment block
(`:1936-1968`) is the best single document on this page: it records why the
caption key is declared on the STATIC container and **moved** onto the generated
`.knob-label`, and why `removeAttribute` is not optional (keeping the key makes
the next language sweep call `applyLabel(container)`, whose `textContent` write
**deletes the entire expanded knob**). Read it before you bind anything to a
knob. Your tip anchors are `#knob-<paramId>` / the `.knob-container`, both live.

**2. The 64 mod-matrix parameters ARRIVE TOO LATE, and no shipped plugin has the
mechanism.** `<div id="mod-matrix-rows">` is **empty in the static markup**
(`:1458`). An `(async function(){...})()` at `:2848` `await`s
`Juce.getNativeFunction('getModSourceNames')()` and `('getModDestNames')()`,
then builds 16 rows with `const prefix = 'modSlot' + i` (`:2873`) and
`container.appendChild(row)`. Because of the `await`, **every one of those 64
anchors is absent from the DOM at `applyI18n()` time**. A `TIP_BINDINGS` entry
for any of them resolves to null, warns `i18n: tip target not found`, and binds
nothing — and `boot-all-uis` will see the warning.

  This is measurable without running anything: **0 of the 64 `modSlot*` IDs
  appear as a whole string literal anywhere in the served root.** They exist only
  as that concatenation.

  O-Prism already has the injected-subtree hook — `localizeSubtree(root)`
  (`:1905`, exposed as `window.__localizeSubtree`, called at 8 sites). **It does
  not solve this.** It loops `applyLabel` over `[data-i18n]` and
  `applyI18nAttributes` over the aria/placeholder/alt attributes. **It does not
  write `data-tip-title` / `data-tip`**, and the mod-matrix builder does not call
  it at all.

  **DO NOT INVENT THE MECHANISM.** Extending `localizeSubtree` to carry tip
  attributes is a canon-shaped change, `scripts/` is the orchestrator's, and a
  one-plugin workaround is precisely what hides a repo-wide hole — the rule that
  has already produced ten gate fixes in this task. **Ship the 107. Report the 64
  with this evidence and stop.** The precedent is exact: M2 reported O-Reed's
  `referencePitch` as page-unreachable for the same reason at a scale of one, and
  did not force it.

**3. `stereoWidth` and `velocityCurve` appear nowhere in the served root** — the
two genuine page-unreachable candidates, on the same footing as the 16 above.
Confirm and report.

**4. Every one of O-Prism's 173 parameters has an EMPTY `label` column.** 0%, the
worst in the suite (O-Bells 69%, O-Formant 34%, O-Wind 28%). **Every unit on this
page must be recovered from the page's own formatter, and the brief requires you
to cite the formatter's file:line for each one.** `bindKnob(paramId, formatFn,
defaultNorm, size)` at `:2370` is where those formatters are passed. Budget for
this: it is the single largest piece of copy-authoring work in the whole stage.

## What M3 must leave behind

- zero-tip count **4 → 0**
- `check-i18n` ALL CHECKS PASS across 43; `boot-all-uis` 43/43, native `title=`
  still **0** repo-wide
- four `tests/ui_tip_render_check.js` — the first runnable gate in each of these
  four plugins
- a decision item for the **64 async mod-matrix anchors**, with the evidence above

## Batch M3 additions to the decision items

13. **`SKIP_PLUGINS` is the cheap recipe for an isolated configure.** Divergence 1.
    It does not fix decision items 5 or 6; it routes around them.
14. **O-Prism's 64 mod-matrix parameters cannot be reached by `TIP_BINDINGS`.**
    The canon writes tip attributes during `applyI18n()`; those anchors do not
    exist yet, and `localizeSubtree` — the page's own injected-subtree hook —
    does not carry tip attributes. Needs a canon decision, not a patch.
15. **Sixteen more host-reachable / page-unreachable parameters** (plus O-Prism's
    two). Running total across M1–M3: **1 + 7 + 18 = 26**. At some point this
    stops being a per-batch finding and becomes a question about the suite.
16. **Checkpoint 5 recedes again.** M3 will add roughly 350+ machine-drafted
    French entries. Repo total after M2 was 3471.

---

# BATCH M3 COMPLETE — 4 of 4, and STAGE M IS DONE

All four landed 2026-08-31. **The zero-tip count went 4 → 0.** Repo-wide
`check-i18n` ALL CHECKS PASS across 43; `boot-all-uis` 43/43 clean, 0 warn,
0 failed; native `title=` still 0 repo-wide.

| Plugin | Version | Commit | Files | Params → controls | Tips | Render gate |
|---|---|---|---|---|---|---|
| O-Wind | 1.18.0 | `8c6e1c97` | 7 | 56 → 50 | 52 | 774/774 |
| O-Bells | 4.3.0 | `31fcd0cf` | 7 | 65 → 63 | 65 | 1024/1024 |
| O-Formant | 1.27.0 | `6c898178` | 8 | 64 → 57 | 57 | 1360/1360 |
| O-Prism | 1.22.0 | `db939f7f` | 6 | 173 → 105 | 107 | 2180/2180 |

**281 tips authored** (274 parameter + 8 chrome, minus double-bindings), every
French entry `reviewed: false`. **5338 render-gate assertions, 0 failures.**
Four new `tests/ui_tip_render_check.js` — **the first runnable gate any of these
four plugins has ever had.**

**Stage M total across M1+M2+M3: 22 plugins, 607 parameters, 556 tips.**

The French worklist is now **3751** repo-wide; M3 added 749 entries across the
four (O-Prism's 262 is the largest single block in the repo).

---

# CORRECTIONS AND CARRIED TRAPS FROM M3

Where this section disagrees with anything above, THIS section wins.

## 1. `check-i18n` READS THE WORKING TREE, NOT HEAD — so a repo-wide count in a shared checkout reports OTHER executors' UNCOMMITTED work as shipped

**This is the batch's most important finding and it is about the orchestration,
not any plugin.** O-Wind finished first, ran
`grep -cE '\[2\] 0 tip\(s\) bound'`, got **0**, and concluded in its report that
*"M3's exit condition is met — all four landed."* **Only O-Wind had committed.**
The other three had authored `i18n.js` into the working tree and had not yet
reached their commit step; the gate saw their files and counted them.

Every gate in `scripts/` reads the filesystem. In a trunk-based checkout with N
concurrent executors, **a repo-wide gate result is a statement about the union of
everybody's uncommitted work**, and no executor can distinguish its own
contribution from its neighbours'.

**The rule: an executor may report the repo-wide number as an observation, but
must never conclude a BATCH is complete from it.** Only `git log` can say that.
Verify with `git log --oneline -- plugins/<Name>` per plugin, never with a
gate count.

This is the eighth instance in this task of a gate certifying something it cannot
see, and the first that crosses session boundaries.

## 2. NO GATE IN THIS REPO SEES A DANGLING TIP BINDING AT RUNTIME — and the M brief said the opposite

The M1 brief states that `boot-all-uis` *"is the only gate that sees a swallowed
binding failure"* and that *"a console warning there is a real failure, not
noise."* **Both halves are false, and the M3 dispatch prompts repeated the error.**

- `scripts/i18n-canon.js:165` — `console.warn(\`i18n: tip target not found: ${selector}\`)`
- `scripts/boot-all-uis.js:141` — `if (m.type() !== 'error') return;`

**A `console.warn` is dropped before it is ever examined.** So a `TIP_BINDINGS`
row that resolves to nothing is invisible to `check-i18n` (which only checks the
key exists in `I18N`, statically), invisible to `check-ui-labels` (no tooltip
awareness at all), and invisible to `boot-all-uis`.

Found by O-Bells and verified at both line numbers. **This is why assertion [1]
of the per-plugin render gate — every `TIP_BINDINGS` selector resolves, as a hard
FAIL — is the only thing standing between this stage and 556 silently dead
bindings.** It is now load-bearing for the whole feature, on all 22 plugins.

**Owner: `scripts/`, i.e. the orchestrator.** Recorded as decision item 17.

## 3. A GREP OF PARAMETER IDs OVER A SERVED ROOT CANNOT ESTABLISH REACHABILITY — it failed in BOTH directions, five times

The orchestrator measured the M3 page-unreachable sets by counting occurrences of
each parameter ID in the served root. **Three executors independently found the
method wrong, and it errs both ways:**

**False negatives — a control reached through a NATIVE-FUNCTION ALIAS.** The ID
never appears; the control is real.

| Plugin | Parameter | The alias |
|---|---|---|
| O-Wind | `referencePitch` | shared panel's master-tune knob → `getMasterTune`/`setMasterTune`, routed at `PluginEditor.cpp:337-355` |
| O-Bells | `tuning_masterTune` | `tuning-panel.js:1012/1035` → `PluginEditor.cpp:506` |
| O-Bells | `tuning_octaveStretch` | `tuning-panel.js:271/976` → `PluginEditor.cpp:486` |

**False positives — a string that is not an anchor.** O-Prism's `tonic` matched
a CSS class family (`.tonic-selector`, `.tonic-label`, `.tonic-value`,
`.tonic-arrow`); `tuningPreset` matched a `getSliderState` argument the page
takes only to *listen* on.

**Consequences for the numbers in the M3 section above:** the claimed 16
page-unreachable across the three smaller plugins is **14**; O-Prism's claimed
107 / 64 / 2 split is **105 / 65 / 3**.

**The same scan produced the running 26 figure across M1-M3. Treat that number as
an upper bound that has never been audited by this method's own critics.**

**The rule: a parameter is page-unreachable only when the ID is absent AND no
native function reaches it. Check the bridge, not just the string.**

## 4. THE MECHANISM FOR A LATE-MOUNTING ANCHOR EXISTS, ON EXACTLY ONE PLUGIN IN 43, AND IT IS ONE LINE

O-Bells binds two anchors inside a lazily-`import()`ed panel that O-Reed and
O-Formant could not. The whole difference:

```js
window.__reapplyI18n = () => applyI18n(uiLanguage);   // index.html:1992
```

declared **outside** the byte-compared canon region, and called by the panel's own
init after it mounts:

```js
if (window.__reapplyI18n) window.__reapplyI18n();      // index.html:3066
```

`grep -rl '__reapplyI18n'` across all 43 served roots returns **O-Bells and
nothing else.**

**It is not the same thing as `localizeSubtree`.** O-Prism's `localizeSubtree`
(8 call sites) loops `applyLabel` over `[data-i18n]` and `applyI18nAttributes`
over aria/placeholder/alt — **it writes no tip attributes.** `__reapplyI18n`
re-runs the whole sweep, tip bindings included.

Cost on O-Bells: one `console.warn` per late selector on the first sweep, which
its gate **pins to exactly those two selectors** rather than relaxing.

**So decision item 14 is no longer an open canon question — it is a shipped,
proven, one-line precedent waiting to be generalised.**

## 5. O-PRISM'S 64 ANCHORS HAVE A SECOND BLOCKER THE DISPATCH DID NOT NAME

The dispatch said the rows arrive after `applyI18n()`. True. But **the rows carry
no per-parameter id even after they arrive** — the columns are `.mod-col-src` /
`.mod-col-dst` / `.mod-col-amt` / `.mod-col-on` inside `#mod-row-<i>`.

**So `__reapplyI18n` alone would not fix O-Prism.** The fix is two things: run the
tip write after injection, *and* give the rows addressable nodes. O-Prism's gate
asserts the evidence rather than asserting about it — `[8]` counts
`document.querySelectorAll('[id^="modSlot"]')` and requires **0**, so if the page
ever grows those ids the gate says so.

O-Prism correctly shipped the 105 and stopped.

## 6. STRUCTURAL CLAIMS IN THE M3 SECTION THAT WERE MEASURED WRONG

The orchestrator's own pre-dispatch measurements, corrected by the executors:

| Claim | Truth | How it was mismeasured |
|---|---|---|
| O-Wind has **1** tab panel | **3** (`#tab-sound` active, `#tab-tuning`, `#tab-effects`); 21 of 52 anchors on the hidden Effects tab | counted `class="tab-content"` literals; O-Wind names them differently |
| O-Formant's page has **3** inline modules | **1**, at `:1402` | 3 is `check-i18n`'s *module* count (inline + `js/main.js` + `js/tuning-panel.js`), not a count of `<script type="module">` |
| O-Prism has **61** JS-expanded knobs | **64** — 60 plain `div.knob-container` plus the four `#lfoN-rate-wrap`, which carry an id as well as `data-knob` | the placeholder grep missed the id-bearing variants |
| `I18N_EXEMPT` column `—` for all four | O-Wind's is **15** | not measured before dispatch |

**None changed an executor's approach**, because every dispatch said "confirm
each, do not take them on faith" and every executor did. That instruction is the
reason these are corrections and not defects.

## 7. FIVE SHIPPED DEFECTS FOUND WHILE WRITING COPY — the stage's highest yield yet

M2 found three. M3 found five, plus one that was fixed:

- **O-Wind: `toneHoleToggle` is DEAD.** `PluginProcessor.cpp:316-319` records the
  tone-hole DSP was never implemented. The switch moves, the automation lane
  moves, nothing is heard.
- **O-Wind: all four FX bypass buttons read INVERTED** against their parameter —
  the face says `On` while `chorusBypass` sits at `Off`. **O-Prism has the same
  defect on five buttons** (`bindBypassToggle`, `index.html:3213`). Two plugins,
  same shape, found independently.
- **O-Formant: three knobs are wired to relays that do not exist.** `js/main.js:288-290`
  asks for `consonantAttackSlider` / `HoldSlider` / `DecaySlider`; `PluginEditor.cpp`
  declares 54 relays and none of those three. The parameters are live and every
  one of the 16 consonant presets sets them; the *controls* are dead.
  (`pattern_webview_native_fn_bridge_gap`.)
- **O-Formant: a session saved at A4 = 442 Hz reopens at 440.** The tuning panel's
  A4 REF and Stretch knobs call `setMasterTune`/`setOctaveStretch`
  (`PluginEditor.cpp:327-345`), which write the `TuningEngine` **directly and never
  the parameter**. `getStateInformation()` does not save them, and
  `setStateInformation()` then pushes the never-updated parameters back into the
  engine (`PluginProcessor.cpp:986-987`).
- **O-Prism: two dead `bindKnob()` calls** (`:2721`, `:2722`) look for
  `#knob-masterTune` / `#knob-octaveStretch`, neither of which exists. Harmless —
  both parameters have working bespoke controls — but `octStretchFmt`'s
  three-decimal formatter is passed to a call that never runs while the readout the
  user sees prints two decimals from a different line.

**FIXED, because it was this feature's own foundation: O-Formant's language
persistence had been dead since v1.26.0.** `js/main.js` imported four *named*
bindings from `./juce/index.js` and no namespace, so `Juce` was unbound,
`initI18n()`'s first statement threw `ReferenceError`, and its own `try/catch`
degraded the feature to session-only behind a `console.warn` **that no gate fails
on**. One added `import * as Juce`. This is
`critical_juce_webview_namespace_vs_postmessage`, and it is the third
gate-blind-to-a-warning finding in this batch alone.

Every one of these went into a tooltip body rather than being described as
intended. **A tip that lies is worse than no tip** has now rewritten ten sentences
in this task.

## 8. THE NEGATIVE CONTROL THAT NEEDED THREE INSTRUMENTS

O-Wind's boundary guard (renderer property 3 — ignore a move between two
descendants of the same anchor). **A post-hoc DOM read passed 774/774 with the
guard deleted. So did a per-frame opacity sampler** (min opacity 1 over 25
frames). `pointerout` and `pointerover` for one move land in the **same task**, so
the tip hides and reopens before the style system settles and before any frame
renders.

**Only a `MutationObserver` on the class attribute sees it** — 10 mutations,
5 hides, with the guard gone. The naive assertion is kept one line below,
explicitly labelled as passing both ways.

Same family as M2 finding 6's static-grep note, one layer deeper: **a control that
samples STATE cannot see a transition that begins and ends inside one task.**

## 9. THE GUARD THAT MUST ASSERT ITS OWN RELEASE — proven, not argued

M2 finding 2 said a `pointerHeld` drag guard "must assert the RELEASE as well as
the hold, or it is a permanent off switch that passes." **O-Prism ran it:**
deleting the `pointerup`/`pointercancel` release gave **1290 FAIL / 890 pass** —
the flag latches on the first click and no tip ever opens again.

The drag-guard *need* also split cleanly along M2's predicted line, and both
branches were measured rather than defaulted: **O-Formant needed none** (all three
drag surfaces call `setPointerCapture` — `main.js:510, :685, :852`, the
O-AnalogEQ shape); **O-Bells needed one** (sliders, FX knobs and the A4 knob all
drag on `mousedown` + `document.mousemove`, with **zero** `setPointerCapture`
calls anywhere).

## 10. THE EDITOR GUARD BELONGS IN `show()`, NOT IN `focusin`

O-Bells: a double-click value editor replaces a readout's text node with an
`<input>` **under a stationary cursor**, and Chromium dispatches a fresh
`pointerover` for the new element. The latch has nothing to say about that — it is
not a focus event. A `focusin`-sited guard passed every static check and failed
the behavioural control.

## 11. THE BLUR 2×2 SPLIT AGAIN, AND EVERY EXECUTOR RAN IT

M2 finding 6 said to run the 2×2 and report the cell rather than assume. M3's
score: **O-Wind and O-Bells and O-Prism → blur load-bearing** (the O-Tremolo
cell); **O-Formant → blur is decoration** (the O-TextureForge cell, an accident of
section order). Kept and labelled in all four.

The static-grep half is now confirmed on **ten** plugins: deleting only
`if (lastInputWasPointer) return;` leaves every `grep -c lastInputWasPointer`
green. O-Prism's returned 3 through an entire failing run.

## 12. A HOVER HARNESS THAT SCROLLS ON VIEWPORT VISIBILITY IS BLIND TO A SCROLL-CONTAINER CLIP

O-Formant, and it is the trap to carry to the next tabbed page. A `hoverAnchor`
that scrolls only when the rect leaves the **viewport** does not notice an element
clipped by its own **scroll container**. A previous anchor's scroll left
`.right-col` mid-way; `glottalRd`'s rect read y=71 against the container's own top
of 90; the pointer landed on the tab bar; and the surface was measured still
carrying the **previous anchor's text** — **84 failures that look exactly like a
copy bug.**

**Fix: scroll unconditionally, and assert `elementFromPoint` lands on the anchor
before every hover.** Same family as
`pattern_rect_gate_cannot_see_paint_order` — a rect comparison cannot see what is
actually under the cursor.

## 13. THE PLANT SEARCH, ON LARGE FRAMES

Every executor searched rather than guessed, and every one of them found the
habitual plant would have reported nothing:

| Plugin | Frame | Habitual 40× plant | First size that overflows |
|---|---|---|---|
| O-Formant | 800×600 | 640 chars → 248 px, **fits** | 1920 chars → 649 px |
| O-Bells | 800×600 | — | 3504 chars → 1236.9 px |
| O-Wind | 900×600 | 4× → 262.5 px, fits | boundary 9×→10× (1460 chars) |
| O-Prism | 1200×800 | — | 3200 chars → 1063 px |

**O-Prism also found the positive control's window is narrow and easy to miss.**
A clamp probe must be taller than `max(H−24−y, y−20)` to fit on neither side and
shorter than `H` to be the clamp rather than the overflow probe again — roughly
390–799 px at mid-height on that page. Its first draft doubled to an **8481 px**
tip, "which is section 6's probe wearing 6b's name"; stepping by 5 reps found the
real one at 1625 chars / 585.7 px.

## 14. "BIND TO THE IDS THE UI ALREADY USES" — the final score is 2 of 18

M3's rows, completing the tally:

| Plugin | Selector half | Target half |
|---|---|---|
| **O-Prism** | **TRUE** — 107 of 107 are ids; the page has no `data-param` at all | **TRUE** for 81 of 107 — `.knob-container` is an inline-flex column whose only child is `#knob-<paramId>`, so the rects are identical |
| O-Formant | false — 45 of 57 knobs carry no id | true for 53 of 57 (the O-Freeze reason: `.knob-wrap` IS the flex column); `.toggle-wrap` matches TWICE, so `closest()` not `querySelector` |
| O-Bells | mixed | mixed |
| O-Wind | mixed | mixed |

**O-Freeze and O-Prism are the only two plugins in eighteen where the naive
reading of T17 holds on both halves** — and they are at opposite ends of the size
range, which is the point: it is a property of how the page was built, not of how
big it is. Check the two halves separately, every time.

## 15. A SECOND HOVER SURFACE CAN ALREADY BE ON THE PAGE

O-Bells' `.hi-fi-toggle` carried its own `:hover`-only `.toggle-tooltip` at
`z-index: 100` — a pre-existing tooltip that would have painted **alongside** the
new renderer. The div and its three CSS rules were deleted and its sentence moved
**verbatim, both languages**, into `tip.highFidelity`'s body, with the now-unused
`label.hiFiNote` key deleted rather than orphaned (assertion 15).

**Grep your page for an existing `:hover` surface before you add one.** Contract
§4 deletes native `title=`; it says nothing about a hand-rolled CSS tooltip, and
`check-i18n` assertion 11 does not look for one.

## 16. `--amend` WAS UNSAFE AGAIN, AND THE SHARED-HEAD RACE FIRED

O-Bells' parent is `8c6e1c97` — **O-Wind's M3 commit landed between O-Bells'
status check and its commit.** Every executor used `<sha>^` rather than `HEAD~1`
and every one was right to. Third batch running, third time this has mattered.

## DECISION ITEMS — added by M3

17. **No gate sees a dangling tip binding.** Finding 2. `boot-all-uis.js:141`
    drops every non-`error` console message and the canon warns at `warn` level.
    The cheapest fix is to promote that one canon line to `console.error`, or to
    have `boot-all-uis` match `i18n:` warnings specifically. **`scripts/` is the
    orchestrator's; this is the eleventh gate fix this task has surfaced and the
    first that the whole T17 feature rests on.**
18. **Generalise `__reapplyI18n`.** Finding 4. One line, shipped on O-Bells,
    absent from 42. It is what O-Reed's `referencePitch`, O-Formant's tuning
    panel and O-Wind's panel all needed. Supersedes decision item 14 as an *open
    question* — the question now is only whether to roll it out, not what to build.
19. **O-Prism's mod matrix needs addressable row nodes as well.** Finding 5.
    `__reapplyI18n` alone is insufficient there.
20. **The 26 host-reachable / page-unreachable figure is unaudited.** Finding 3
    shows the scan that produced it errs in both directions. The real count is
    unknown; M3's own contribution is 14 + 3 = **17**, after correction from 18.
21. **Two plugins ship inverted bypass buttons** (O-Wind 4, O-Prism 5). Same
    shape, found independently. Host-visible either way it is fixed.
22. **O-Formant's A4 tuning is lost on session reopen.** Finding 7. Host-visible.
23. **`scripts/serve-ui.js:320` seeds the stub from `params.tsv`'s `id` column**,
    but O-Formant's page asks for `<id>Slider`. Nothing matches, so
    `built.seedFrom` reports `param-dump (64 rows)` while `__stubReport.rangesFrom`
    reports `neutral-defaults` and every readout renders `0.50`. Two fields
    disagreeing is the tell. Not blocking; `scripts/` is the orchestrator's.
24. **`check-ui-labels.js:1022`** prints `${seenVisible.size} of ${allPaths.length}`
    — a union across states against a single-state snapshot. Read "123 of 123" by
    coincidence before O-Bells' commit and "123 of 122" after, purely because one
    never-visible node was removed. Not an assertion, not a blocker.
25. **Checkpoint 5 is now 3751 entries.** M3 added 749. No native speaker has read
    any of them, on any of the 43 plugins.

## STILL NOT VERIFIED, on all four

No DAW test — `auval` and headless Chromium only, never a real WKWebView. The
Standalone `.app` is stale on all four (`build-and-install.sh` builds VST3 + AU
only). Checkpoint 4 (host session save/reopen) reasoned, not executed — **and
O-Formant's finding 7 is now a live reason to actually run it.** Windows/WebView2
font metrics remain the standing hardware-blocked deferral, and tooltips are new
surface for it: French already wraps taller inside the fixed cap on every one of
the four (O-Prism 123.9 → 139.3 px), so a wider face shows there first.
