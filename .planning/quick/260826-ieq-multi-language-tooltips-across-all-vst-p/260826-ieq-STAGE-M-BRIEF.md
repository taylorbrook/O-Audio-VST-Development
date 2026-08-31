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

5. **Decision item, carried up:** French tip ranges keep the readout's DECIMAL
   POINT (`0.1 à 100 ms`, not `0,1`), because the readout formats with a point
   in both languages under D-03. A reviewer must change both or neither.

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
