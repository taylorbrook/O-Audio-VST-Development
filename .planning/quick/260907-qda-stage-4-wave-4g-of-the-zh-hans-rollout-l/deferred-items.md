# Wave 4g — deferred items and structural findings

Collected from all seven tasks of `260907-qda`. **Nineteen items: G1–G9 new this wave, I1–I10
re-inherited unchanged.** The wave that closes the zh-Hans corpus, so the forward-looking section at
the end is short by design — what remains is quality and coverage work, not volume.

Every item carries a **grep-able token** rather than a line number, because line numbers drift in
days and a token survives an edit by another session.

---

## New this wave — G1 to G9

### G1. Three frozen render-harness version literals — a suite-wide sweep, not a per-plugin oddity

Three of the six plugins carry a hard-coded version pair in
`plugins/<Name>/tests/render-harness/CMakeLists.txt` that has **never tracked the plugin version**:

| plugin | frozen literal | plugin shipped today | drift |
|---|---|---|---|
| O-simpleAdditive | `JucePlugin_VersionString="1.0.2"` / `0x10002` | 1.3.0 | 4 minors |
| O-simpleSampler | `JucePlugin_VersionString="0.1.0"` / `0x000100` | 1.5.0 | a major |
| O-simpleSubtractive | `JucePlugin_VersionString="1.0.0"` / `0x10000` | 1.5.0 | 5 minors |

**They are dead literals, not `${VAR}` mirrors, so a bump does not desynchronise them any further
than they already are.** That is exactly why nobody noticed. Contrast **O-simpleGrain**, whose
harness reads `${OSIMPLEGRAIN_VERSION}` — a live reference, which is why K6's hex mirror
(`OSIMPLEGRAIN_VERSION_CODE`) had to move with the string and did.

Out of scope in a localization commit (wave 4e D6 puts the render-harness trees out of scope), and
editing one inside a commit whose subject is a caption table would be a change nothing asked for.
**Three of the three harness trees examined this wave carry the pattern, which makes a suite-wide
sweep a measurement rather than a guess.**

**Grep-able tokens:** `JucePlugin_VersionString="1.0.2"`, `JucePlugin_VersionString="0.1.0"`,
`JucePlugin_VersionString="1.0.0"`.

### G2. The unscanned-module class — measured on four of six, and the copies have FORKED

**This EXTENDS s71 D2; it does not merely inherit it.** Four module JS files on this wave's own
plugins are opened by **no gate in this repo**:

| file | carriers | user-facing English | copy state |
|---|---|---|---|
| `modules/webview-drop-streaming.js` | O-simpleGrain, O-simpleSampler | **17 distinct strings across 22 `opts.showToast(` call sites**, every one of which renders **English on the Chinese page** | **byte-identical**, sha256 `349a0c28…`, re-fired on both carriers |
| `modules/preset-manager.js` | O-Polystutter | 7 strings, mostly dead paths | **406 lines, sha1 `0142c1da…`** |
| `js/modules/preset-manager.js` | O-Orbit (embedded per `CMakeLists.txt:43-48`) | same 7 strings | **447 lines, sha1 `d15e751b…`** |
| `Source/ui/public/modules/preset-manager.js` | O-Orbit — **not embedded, not served** | same | same sha as above |

**The new fact s71 D2 does not contain: the `preset-manager.js` copies have FORKED.** s71's table
treats it as one file with eighteen consumers. It is at least two files, **41 lines apart**, and
O-Orbit carries two copies of its own version in one tree — one served, one not.

**The blindness is DIRECTORY-SCOPED and therefore repo-wide.** `check-i18n`'s `[12]` clause scans
`js/app.js` (and `js/parameter-bindings.js` on O-Polystutter) and reports **PASS** on the very same
plugin whose `modules/preset-manager.js` it never opens. **This is blind on all 44 plugins**, not
only on the four this wave happened to look at.

**Scope-out reason, written down:** keying an unkeyed toast layer needs a calibrated gate that does
not exist (wave 4c D4 — a gate written mid-localization is a gate nobody has calibrated); the fix
must land on every forked copy at once; and shipping it inside a caption-table commit would put four
unrelated plugins' UIs in one change. **Widening the `[12]` scan past the `js/` directory is the root
fix and is a rollout of its own.**

**Grep-able tokens:** `opts.showToast('Scanning folder…')` · `createPresetBar` in
`modules/preset-manager.js`.

### G3. `'Loaded Preset'` — an unkeyed, unexempt English string that DOES reach two pages

`this.currentPreset = result.name || 'Loaded Preset'` in both forked `preset-manager.js` copies
(O-Polystutter `:283` → `#preset-name-text`; O-Orbit `:297` → `#preset-name`). Reachable **only after
a successful file load**, so no static scan sees it, no `[data-i18n]` covers it, and no state in
`i18n-states.json` fires it.

**Its sibling `'Default'` IS an `I18N_EXEMPT` entry and this one is not**, so the exemption list is
**incomplete** rather than the string being deliberately English. The same defect in two forked
files, filed independently by Tasks 5 and 6.

**Grep-able token:** `'Loaded Preset'`.

### G4. Six single-sited glossary roots need a glossary-level decision

Every one was corrected **per-entry** with a reasoned `termNote` (entry-scoped, N4) and **not forked
silently**. Every one had its corpus site count checked **before** the note was written — D1's
general lesson, applied — and on all six the count was **one or two**.

| root | as shipped | the problem | sites |
|---|---|---|---|
| `'Morph Pad'` → `变形面板` | a morph **panel**, a UI surface | the English names a synth **pad**; rendered `变形音垫` | **1** (O-simpleAdditive) |
| `'Organ'` → `管风琴` | specifically a **pipe** organ | the preset's own body names a Hammond drawbar registration; rendered `电风琴` on 3 entries. **O-Bells is the independent confirmation** — its body uses the same word to name the *pipes* of one | **1** (O-simpleAdditive, 3 entries) |
| `'granular fire'` → `颗粒触发` | reads *Fire* as the verb **to trigger** | the page's own tooltip body names the crackling-**fire** recording; rendered `颗粒之火`. Three independent blind readings all returned the flame | **1** (O-simpleGrain) |
| `'load your own'` → `载入自己的` | ends on a possessive with **no head noun** — grammatically incomplete in Chinese | rendered `载入自己的素材` on **both** sites with the same head noun so the plugins do not diverge | **2** (O-simpleGrain, O-simpleSampler) |
| `'sub'` → `['低音','超低频']` | both are low-frequency **BAND** words | this plugin's is a **sub-oscillator** mix level; it back-translated as "Bass". Needs either a second accepted member for the oscillator sense or a scope note saying the root is band-only | 3 sites, 2 band + **1 oscillator** |
| `'taper'` → `渐变` | reads as **"Gradient"** to a caption-only reader | **ACCEPTED, not forked** — the shipped corpus root, and no second gradient-like control exists on O-simpleGrain's page to collide with. **Recorded because any plugin pairing a Taper caption with a gradient control would have a real collision** | corpus root |

**A shared-script edit is outside a per-plugin task's path scope**, which is why all six are carried
rather than made. **This is the same shape as 4f D1 and it is now six items deep — the pattern is
that a root derived from ONE plugin's caption+title pair looks measured (2 occurrences) and had no
second site to check it against.**

**Grep-able tokens** in `scripts/i18n-zh-glossary.js`: `变形面板` · `管风琴` · `'granular fire':` ·
`'load your own':` · `'sub':` · `'taper':`.

### G5. `routing-label` is a SHARED CLASS and the carry-forward names one carrier

**This is a correction to the SCOPE of 4e D7 / 4f D12**, both of which record the French wrap as
*"a pre-existing French wrap on O-simpleFM"*. Measured by Task 4: **five files across three
plugins** carry the class —

- **O-simpleFM** — markup + CSS
- **O-simpleSubtractive** — markup + CSS *(in this wave's own set)*
- **O-simpleGrain** — CSS only, **no markup carrier**

**Both markup carriers wrap in French and neither wraps in English or Chinese.** The consequence was
concrete: **O-simpleSubtractive's `wrap-count` baseline is 1, not 0**, and holding it at 1 was the
correct criterion. Re-scope the finding to the class, attach the carrier list, and take any future
decision about it **once for all carriers**.

**Grep-able token:** `routing-label` in `plugins/*/Source/ui/public/index.html`.

### G6. F2's "eight two-language gate files" is an UNDERCOUNT — it is thirteen

4f's F2 tabulates eight files and records the wider census as `13 at planning time → 8 after`. Fired
at this wave's close-out **with 4f's own command**, it returns **13**:

| file | `fr` hits (comment-stripped) | last committed |
|---|---|---|
| `O-Bass/tests/ui_tip_render_check.js` | 2 | 2026-09-03 |
| `O-Bells/tests/ui_tip_render_check.js` | 3 | 2026-09-05 |
| `O-Chorus/tests/ui_tip_render_check.js` | 2 | 2026-09-03 |
| `O-Emulator/tests/ui_tip_render_check.js` | 3 | 2026-09-05 |
| `O-Freeze/tests/ui_tip_render_check.js` | 2 | 2026-09-03 |
| `O-MicrotonalSampler/tests/ui_tip_render_check.js` | 2 | 2026-09-04 |
| `O-ReverseDelay/tests/ui_frontend_check.js` | 2 | 2026-09-06 |
| `O-SimpleReverb/tests/ui_tip_render_check.js` | 3 | 2026-09-05 |
| **`O-Bitrot/tests/ui-stub/juce-stub.js`** | 1 | **2026-08-26** |
| **`O-Octagon/tests/ui-stub/juce-stub.js`** | 1 | **2026-08-27** |
| **`O-ReverseDelay/tests/ui-stub/juce-stub.js`** | 1 | **2026-08-26** |
| **`O-SpectralShaper/tests/ui-stub/juce-stub.js`** | 2 | **2026-08-28** |
| **`O-Tapestop/tests/ui-stub/juce-stub.js`** | 1 | **2026-08-26** |

**Nothing moved.** The first eight are F2's table verbatim. The bolded five are a `ui-stub/juce-stub.js`
class F2's table omits, and every one was last touched **weeks before wave 4f**, so they were on the
tree at 4f's close-out too. **F2's `8` describes the files it enumerated, not the census it cites.**

**None of the thirteen belongs to any wave-4g plugin** — none of the six owns a gate file at all —
**and nobody has audited any of them.** Every one belongs to a plugin that is already three-language,
so their gates either derive the language list correctly or are vacuously green on a page whose third
language they never visit. **This is the whole remaining N12 surface and it is 13 files, not 8.**

**Grep-able token:** the census command in 4f's verify arm 12.

### G7. A pre-existing `auval` warning on O-simpleAdditive — new to the ledger, not new to the code

```
Name: Scan LFO Rate
Values: Minimum = 0.0100000, Default = 0.5000002, Maximum = 20.0000000
WARNING: retrievedValue = 0.328712 (was 0.328712), Parameter did not retain default value when set
```

**Not attributable to this wave, and not attributable to D10's two known warnings either** — those
belong to O-Bowed and O-Wind.

`PluginProcessor.cpp:84-87` declares `NormalisableRange<float> { 0.01f, 20.0f, 0.0f, 0.3f }` with
default `0.5f`. **Skew 0.3.** `((0.5 − 0.01) / 19.99) ^ 0.3 = 0.328712` — the retrieved value is the
**correct normalized form of the 0.5 Hz default**, and `auval` compares it against the raw `0.5`.
`(was 0.328712)` — the two figures are identical, which is the giveaway that the round-trip is exact.

**`plugins/O-simpleAdditive/Source/PluginProcessor.cpp` was last committed at `b1082fc0`, 2026-08-27**,
and appears in neither of Task 1's commits. This wave touched no parameter, range, type, state format
or audio path on any of the six.

**Structural to any skewed AU parameter. Worth a look the next time O-simpleAdditive's parameter
layout is open; it is not a localization item and it blocked nothing.**

**Grep-able token:** `Scan LFO Rate` in `plugins/O-simpleAdditive/Source/PluginProcessor.cpp`.

### G8. PLUGINS.md's whole-suite miss is now a PATTERN — twice in a row, and 38 rows unaudited

**All six of this wave's rows disagreed with their own CMakeLists before the wave started**, every
one by exactly one patch, all six from the **same 2026-09-03 suite-wide French hover-help rename
(`260903-ukp`)** — the same commit that left O-GrainScatter's row **two minors** behind in wave 4f.

**The mechanism is a suite-wide edit that bumps every CMakeLists and updates no row.** It has now
happened twice consecutively and been caught twice only because a localization wave happened to
touch the same plugins.

**Nobody has audited the other 38 rows.** The reconciliation is one command and it is cheap:

```bash
for p in $(ls -d plugins/*/ | xargs -n1 basename); do
  V=$(perl -ne 'if(/^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1";exit}
                if(/^\s*set\s*\(\s*\w*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1";exit}' plugins/$p/CMakeLists.txt)
  R=$(grep -oE "^\| $p \| [^|]+ \| [0-9]+\.[0-9]+\.[0-9]+" PLUGINS.md | grep -oE "[0-9]+\.[0-9]+\.[0-9]+$")
  [ -n "$V" ] && [ -n "$R" ] && [ "$V" != "$R" ] && echo "  $p cmake=$V registry=$R DISAGREE"
done
```

**Use the two-arm reader** — a one-arm reader returns EMPTY on the `set()`-variable shape and will
report a false AGREE by comparing nothing.

**Grep-able token:** `260903-ukp` in the git log.

### G9. O-Strata arrived fully localized mid-wave, from another session

Commit **`0e838512`** — *"feat(O-Strata): Stage 1 foundation — fork of O-Prism v1.24.0, 48 geometry
params, wavetable library removed"*, followed by `f25be4d9` and `b80b2d56`.

**Because it is a fork of an already three-language plugin, it arrived with a complete `i18n.js` at
`reviewed: 'bt'` without any localization work being done on it.** Consequences, all measured:

- **The corpus closes at 44 of 44 / 5789 of 5789**, not 43 of 43 / 5441. `4621 + 348 + 820 = 5789`.
- **Both repo-wide lints' `exit 2` baseline is GONE** — the standing cause was O-Strata's missing
  `i18n.js` (4f D5 / K2). Both now exit **0** with **0 unreadable**.
- **`boot-all-uis` moved 43/43 → 44/44**, 3842 → **4701** text-bearing elements, 798 → **804**
  aria-label. `3842 + 859 = 4701` and `798 + 6 = 804` exactly, so the entire delta is O-Strata's own.
- **The denominator MOVED WITHIN THE HOUR.** Task 5 read `5816`; Task 6 read `5789`, as that session
  trimmed the table. **Any close-out arithmetic pinned to a constant is wrong by the time it is
  written — read it live and state the reading with its timestamp.**
- **`check-i18n` repo-wide FAILED at Task 5's preconditions** (`[10]` on O-Strata, 21 uncovered
  option nodes) **and PASSED at its self-check**, because that session fixed it in the interval.
  Both readings are recorded because either alone misleads.

**Nothing under `plugins/O-Strata/` was staged by any of this wave's thirteen commits, and its
PLUGINS.md row was not edited.**

**Grep-able token:** `O-Strata` in the emitter's per-plugin table.

---

## Re-inherited unchanged — I1 to I10

### I1. 4f D1 — `'dist lpf': ['失真低通']` should read `['距离低通']`

`scripts/i18n-zh-glossary.js:433`. The only site in the corpus (O-GrainScatter) means **distance**,
not distortion. **Still the oldest open corpus-level item.** Not this wave's plugins.

**But its general lesson was applied on all six of this wave's plugins and it paid: G4 above is six
more roots of the same shape.**

### I2. 4f D2 — O-Reed's 20 missing and 7 dead native-function registrations

A C++/JS bridge defect on the tuning tab, not a localization one. **NOT INHERITED** — O-Reed is not
in this wave and no task opened it. Unchanged.

### I3. 4f D3 — two stale `d848337a` floors on O-Reed

`.tonic-label` (41.59 px) and `.interval-list-header` (24 px), never re-measured with the floor
removed (F7). **NOT INHERITED**, same reason. **No gate in this repo can see either of them.**

### I4. 4f D4 — the 17-of-37 shared-module coverage hole

**EXPLICITLY NOT INHERITED.** None of the six is a `scala-tuning-engine` consumer — verified two ways
on all six (CMake embed grep, and the absence of any tuning tab or `tuning-panel.js`). Unchanged at
20 visible / 10 never-visible / 7 never in the DOM. Closing it needs states that change the generator
type and select the Rotation visualisation, which no wave has budgeted.

### I5. 4f D6 / 4e D4 — the glossary divergence report, SIX entries

`WebGL 不可用` vs `不支持 WebGL` · Scatter and Diffusion · `通过长度` for *pass length* · `分割` for
*Divisions* · `键位` for *keyswitch* · `调制` for *MOD*.

**This wave added NO SEVENTH.** Task 5 considered `Rolloff` and deliberately did not fork it — the
reasoning turns on the French root naming a numeric quantity the Chinese root does not. **The wave's
22 termNotes are per-page sense exemptions against correct roots, which is a different thing from a
divergence** (a divergence is a corpus-wide decision; a termNote is entry-scoped).

### I6. 4f D7 / 4e D5 — the Z6 budget backfill

`i18n-zh-lint` prints it on every run: **`3 of 552 glossary terms carry a measured budget; 549 are
UNBUDGETED and Z6 is inert on them`**. **Re-fired live at this wave's close-out: unchanged.**

**This wave added none**, for the predicted reason and now for the sixth consecutive wave: every
geometry finding was a line-box growth wanting a **ratio**, or a shrink an existing **floor** already
absorbed. Neither wants a character budget.

### I7. 4f D8 / s71 D3 — `modules/registry.yaml` `used_by` for scala-tuning-engine is STALE

Names O-Bells, O-Formant, O-IntonationPad (forks with private copies, **none live consumers**) and
omits O-Reed, O-Wind, O-MicrotonalSampler. **Left as found. DRIVE NOTHING FROM IT until it is rebuilt
from the CMake grep.** No task in this wave read it; every consumer question was answered from the
CMake grep.

### I8. 4f D9 — the two O-Contrabass orphan bundles, and they are NOT variant shadowing

**RE-CONFIRMED off this wave's own `auval -a`:**

```
aumu OCb5 OuDv  -  O-Contrabass-pre-2-5-dev
aumu OCbP OuDv  -  O-Contrabass-pre-port
aumu OCbs OuDv  -  O-Contrabass-dev
```

Three distinct triples, three distinct product names. **They do not share the AU triple with the
current O-Contrabass and cannot shadow it.** `build-and-install.sh`'s Phase 4 sweep is keyed on
`<Name>` and `<Name>-dev` only and **structurally cannot see a third product name**, so they persist
until removed by hand. **No build in this wave emitted `⚠ Sweeping ALTERNATE-variant`** — all six task
summaries record the grep count as 0.

### I9. 4f D11 — `reviewed: 'native'` is OPEN on all 5789 corpus rows

This project has no native Simplified Chinese reader. **A disclosed quality level**, printed by lint
rule R1 on every run and stated in all six CHANGELOGs. This wave's 820 rows join the standing set.
**A blocker for nothing.**

### I10. 4f D12 — the rest, re-inherited unchanged

- **4e D2 — O-Bells' two late tip bindings** (`#ref-pitch-knob`, `#octave-stretch`). **Used as this
  wave's boot census control: 2 late before, 2 late after, 0 DEAD across 0 plugins.** That pair is
  what makes the `0 DEAD` beside it evidence rather than a number.
- **4c D4** — O-Chorus's inert CJK tail; O-Gain's and O-IntonationPad's missing tip gates. Not this
  wave's plugins.
- **s71 D4** — O-MicrotonalSampler's diverged 317-line private copy, untouched by decision.
- **s71 D6** — `.interval-list-header` wraps in English below ~130 px and the shared CSS still cannot
  carry the floor: five consumers, four different measured English boxes.

---

## Structural findings worth carrying, measured this wave

**S1. M8's leaf line-box ratio table is FACE-SCOPED, not machine-scoped.** Its 11 px row records a
12 px content box; both 11 px leaves on O-Orbit's page measure **13**, because that page's Latin
resolves to **Times** where the pages M8 was measured on resolve to **Times New Roman**. Every ratio
in this wave was derived from the element's own English content box rather than looked up, which is
why they are right. **A later wave that looks a ratio up will be wrong on any page whose Latin face
differs.**

**S2. `system_profiler` disagreed with Chromium on this machine, in the direction that would have
caused a regression.** The M5 table reports `Times` **0** and `Times New Roman` **4**; Chromium
resolves the bare `serif` to **`Times`** and names it that through `CSS.getPlatformFontsForNode`.
**Naming `Times New Roman` first — which the table alone would have licensed — would have moved both
Latin arms onto a different face while "fixing" the Chinese one.** F5 is now a measured claim about a
specific repair, not an inherited warning.

**S3. K16's "nineteen click states" arithmetic is wrong by at least one.** O-simpleSampler's fourth
state is an `eval`
(`window.__stubStates.combos.get('pitchMode').setChoiceIndex(1)`), not a click. `elementFromPoint` at
a target's centre is meaningless for a state with no target, so that state was asserted on its
**effect** alone. **Any executor should re-read its own `tests/i18n-states.json` rather than inherit
a count.** (K16 was TRUE on O-simpleSubtractive, checked before anything was asserted on rects.)

**S4. The emitter has NO product-name control in this version.** `grep -cin "product"
scripts/i18n-zh-backtranslate.js` returns **0**. Four tasks were directed to read a line that is never
printed; all four adjudicated by hand instead. **Either the rule text or the script needs correcting**
— an executor following the rule literally must choose between reporting a false zero and inventing a
procedure.

**S5. `.settings-label` carries `min-width: 0`, which is NOT "no floor" (R8/F7).** The flex-item
default is `auto` (content size), so `0` explicitly permits shrinking **below** content. Not
load-bearing on O-simpleSampler — the finding on that selector was a height and the width never moved
on any arm — but it is a live instance of the trap. **Grep-able token:** `min-width: 0;` in
`plugins/O-simpleSampler/Source/ui/public/css/styles.css`.

**S6. `.tour-caption`'s reserved flex line is a TWO-SITE pattern and will recur.** O-simpleAdditive
(256.34 px collapse, floored at a measured 311.52 px) and O-simpleSubtractive (258.3 px collapse,
floored at the 46% cap). Both are `.preset-tour` / `.tour-buttons` / `.tour-caption` with
`flex-wrap: wrap` and a percentage `max-width`. **Every remaining plugin in the O-simple family with
a preset tour will hit it, and the floor is cheaper expressed in the cap's own unit than as a measured
pixel.**

**S7. `tests/i18n-states.json` state names carry MEASUREMENTS in their prose, and one is 183.91 px
stale.** O-simpleGrain's Pitched Buzz state name records *"997.22 px natural NOWRAP"*; the real figure
today is **813.31 px**. **The number is a description, not an assertion, so nothing fails — which is
exactly why it drifted.** Every wave that inherits the file inherits a wrong number. **Grep-able
token:** `997.22 px natural NOWRAP`.

**S8. Two `label.kbdHint` findings on O-simpleAdditive.** The en and fr arms write the QWERTY key run
with ` ` hair spaces that the file's own comment calls load-bearing; the zh row uses plain spaces,
which is the glossary root's form **and** the form O-simpleFM and O-simplePhysicalModelSynth already
ship — so this file was the outlier, not the Chinese. Separately, **the glossary root is stored
lowercased**, so it lowercases a run of PHYSICAL KEY names; `normZh` lowercases both sides so nothing
fires, but a plugin copying the root verbatim would ship lowercase key names that do not match the
keys. **Grep-able tokens:** `点击琴键或使用电脑键盘` · `click the keys or use your computer keyboard`.

---

## Explicitly for the next wave

**The volume work is done.** 44 of 44 plugins, 5789 of 5789 rows, 0 below the ship bar, both
repo-wide lints exit 0, no plugin still two-language. **There is no wave 4h.** What remains is
quality, coverage and hygiene, and it is short enough to list completely:

1. **G6 / F2 — the thirteen two-language gate files.** Not eight. Every one belongs to an
   already-three-language plugin, none has ever been audited, and this is the **entire remaining N12
   surface**. Start by deciding for each whether its language list is derived or hard-coded.
2. **I1 / 4f D1 — the wrong glossary root `'dist lpf'`.** `['失真低通']` → `['距离低通']`. One line,
   the oldest open item, and outside every per-plugin task's path scope — which is why five waves have
   not made it. **It needs its own commit.**
3. **G4 — five more single-sited roots**, plus the `'taper'` warning. Same shape as D1: derived from
   one plugin's caption with nothing to check against. **A glossary-level pass over every root with a
   corpus site count of 1 would close D1 and G4 together and is the highest-value remaining
   localization work.**
4. **I5 — the glossary divergence report, still six entries.** Each is a corpus-wide decision nobody
   has taken. Unchanged for three waves.
5. **I6 — the Z6 budget backfill, `3 of 552`.** Six waves have added none, every one for the same
   correct reason. **Either fill them from the accumulated `check-ui-labels` zh-arm data, or say
   Z6 is inert by design and stop printing the line as if it were a gap.**
6. **G2 / s71 D2 — widen `check-i18n`'s `[12]` module scan past the `js/` directory.** It is blind on
   all 44 plugins. Then key `webview-drop-streaming.js` (17 English strings on two Chinese pages) and
   **reconcile the forked `preset-manager.js` copies before keying either**, or the fix ships twice
   and diverges again.
7. **I7 / s71 D3 — rebuild `modules/registry.yaml` `used_by` from the CMake grep**, then it can be
   driven from.
8. **G1 — the frozen render-harness version literals.** Three of three examined carry one. Sweep the
   suite; decide once whether these should track the plugin version or be deleted.
9. **G8 — audit the other 38 PLUGINS.md rows** against their CMakeLists with the two-arm reader, and
   consider whether a suite-wide version bump should update the registry in the same commit. **Twice
   in a row is a pattern.**
10. **G3 — add `'Loaded Preset'` to the `I18N_EXEMPT` list, or key it.** Its sibling `'Default'` is
    already exempt, so the list is incomplete rather than the string being deliberately English.
11. **G7 — the O-simpleAdditive `Scan LFO Rate` skew warning**, next time that parameter layout is
    open. Same family as D10's O-Bowed warning; neither is a localization item.
12. **S1 / S2 — never look a line-box ratio up and never read a `system_profiler` zero as absence.**
    Both were proved this wave, both in the direction that causes a regression rather than a
    false alarm.
