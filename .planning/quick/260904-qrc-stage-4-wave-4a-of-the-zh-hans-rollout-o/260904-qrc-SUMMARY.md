---
phase: quick-260904-qrc
plan: 01
subsystem: i18n
status: complete
tags: [i18n, zh-Hans, stage-4, wave-4a, O-Prism, O-Comp, O-Freeze, O-Texture, O-Bass, O-AnalogSaturation, geometry, gates]
requirements: [ZH4A-01, ZH4A-02, ZH4A-03, ZH4A-04, ZH4A-05, ZH4A-06, ZH4A-07, ZH4A-08]
commits:
  - 3e95ffc6   # T1 O-AnalogSaturation, table at 'mt'
  - 4bcec08c   # T1 O-AnalogSaturation, 谐波 correction after round 1
  - da1168a7   # T1 O-AnalogSaturation v1.5.0, promotion + ship
  - a4a21d17   # T2 O-Prism, table at 'mt'
  - c6044e53   # T2 O-Prism, 24 rows re-authored after round 1
  - e88ec412   # T2 O-Prism v1.24.0, promotion + ship
  - 0785755e   # T3 O-Comp, table at 'mt' + tip gate derived
  - 5a3f8ab3   # T3 O-Freeze, table at 'mt' + the Han-wrap fix
  - bfe6af10   # T3 O-Bass, table at 'mt' + two bare-generic stacks
  - 1d48fe87   # T3 O-Texture, table at 'mt' + tip gate derived
  - d08672b6   # T3 O-Texture, XY 控制板 after round 1
  - 55857b7c   # T3 O-Bass, alt.botanical after rounds 1 and 2
  - bbd2208e   # T3 O-Comp v1.9.0, promotion
  - 698ea2bd   # T3 O-Freeze v2.5.0, promotion
  - 462d0991   # T3 O-Texture v0.5.0, promotion
  - 7ef20e1f   # T3 O-Bass v1.7.0, promotion
  - 051b9bcc   # T3 PLUGINS.md, six rows corrected from CMakeLists
key-files:
  modified:
    - plugins/O-AnalogSaturation/Source/ui/public/js/i18n.js
    - plugins/O-AnalogSaturation/Source/ui/public/index.html
    - plugins/O-AnalogSaturation/tests/ui_tip_render_check.js
    - plugins/O-Prism/Source/ui/public/js/i18n.js
    - plugins/O-Prism/Source/ui/public/index.html
    - plugins/O-Prism/Source/ui/public/css/wavetable-editor.css
    - plugins/O-Prism/tests/ui_tip_render_check.js
    - plugins/O-Comp/Source/ui/public/js/i18n.js
    - plugins/O-Comp/Source/ui/public/index.html
    - plugins/O-Comp/tests/ui_tip_render_check.js
    - plugins/O-Freeze/Source/ui/public/js/i18n.js
    - plugins/O-Freeze/Source/ui/public/index.html
    - plugins/O-Texture/Source/ui/public/js/i18n.js
    - plugins/O-Texture/Source/ui/public/css/ouaricon-naturalist.css
    - plugins/O-Texture/tests/ui_tip_render_check.js
    - plugins/O-Bass/Source/ui/public/js/i18n.js
    - plugins/O-Bass/Source/ui/public/index.html
    - PLUGINS.md
  untouched_deliberately:
    - scripts/i18n-zh-glossary.js   # two roots are REPORTED as defective, not edited — see "Two glossary roots"
    - scripts/i18n-zh-lint.js       # the Stage-4 gate flip (exit 2) is not this task's to make
actuals:
  tokens: 367000
  tasks: 3
  commits: 17
---

# Stage 4 wave 4a of the zh-Hans rollout (COMPLETE)

**Six plugins ship English, French and Simplified Chinese.** O-AnalogSaturation
**v1.5.0**, O-Prism **v1.24.0**, O-Comp **v1.9.0**, O-Freeze **v2.5.0**,
O-Texture **v0.5.0**, O-Bass **v1.7.0** — built, installed and auval-clean. The
repo-wide ship-bar view reads **1338 zh rows / 1338 `bt` / 0 `mt`** across ten
localized plugins.

| | O-AnalogSat | O-Prism | O-Comp | O-Freeze | O-Texture | O-Bass |
|---|---|---|---|---|---|---|
| entries / rows | 20 / 27 | **267 / 375** | 39 / 49 | 36 / 51 | 31 / 43 | 29 / 35 |
| at `reviewed: 'bt'` | 27/27 | 375/375 | 49/49 | 51/51 | 43/43 | 35/35 |
| version | 1.4.1 → **1.5.0** | 1.23.1 → **1.24.0** | 1.8.1 → **1.9.0** | 2.4.1 → **2.5.0** | 0.4.1 → **0.5.0** | 1.6.1 → **1.7.0** |
| states driven | 2 | **23** | 4 | 2 | 2 | 3 |
| stacks tailed | 6 | **18** + 1 UA | 6 + **2 canvas** | 5 | 6 (external) | 5 + 2 repaired |
| geometry FAILs → 0 | 12 → 0 | **33 → 0** | 4 → 0 | 4 → 0 | 2 → 0 | 3 → 0 |
| reverse rounds | 2 | 2 | 2 | 2 | 2 | **3** |
| rows re-authored | 3 | **24** | 0 | 0 | 1 | 1 |
| `auval -v` | SUCCEEDED | SUCCEEDED | SUCCEEDED | SUCCEEDED | SUCCEEDED | SUCCEEDED |

**Every requirement is met.** ZH4A-01 through ZH4A-06 (the six plugins), ZH4A-07
(the two gate-file repairs — and two more the census could not see), ZH4A-08
(PLUGINS.md, six rows corrected from the CMakeLists values).

`reviewed: 'native'` stays **OPEN** on all six and is not a blocker. **This
project has no native Chinese reader.** A disclosed quality level, printed by
lint rule R1 on every run and stated in all six CHANGELOGs.

## The end-of-batch auval sweep

Deferred through all three tasks and run **once**, cold, after the last install.
Triples read off `auval -a`, never guessed — and the plan's own assumption was
wrong, which is the argument for the rule:

| Plugin | Version | Triple | Verdict |
|---|---|---|---|
| O-AnalogSaturation | 1.5.0 | `aufx OaSa OuDv` | **AU VALIDATION SUCCEEDED** |
| O-Prism | 1.24.0 | `aumu OuPr OuDv` | **AU VALIDATION SUCCEEDED** |
| O-Comp | 1.9.0 | `aufx OuCp OuDv` | **AU VALIDATION SUCCEEDED** |
| O-Freeze | 2.5.0 | `aufx OFCR OuDv` | **AU VALIDATION SUCCEEDED** |
| O-Texture | 0.5.0 | `aumu OuTx OuDv` | **AU VALIDATION SUCCEEDED** |
| O-Bass | 1.7.0 | `aufx OBas OuDv` | **AU VALIDATION SUCCEEDED** |

**O-Texture is an instrument (`aumu`), not an effect.** The plan said "O-Prism is
an instrument, the other five are effects". Only `-dev` bundles on disk for all
six; the alternate-variant sweep found no orphan.

---

# ══ STRUCTURAL FINDINGS — READ BEFORE WAVE 4b ══

**Six more waves will copy whatever this one establishes.** Everything below
cost measurement to find.

## 1. THE CJK TAIL MUST GO *BEFORE* THE TRAILING GENERIC — and O-Chorus's does not

**Chromium resolves a bare `serif` or `sans-serif` against the DOCUMENT'S
LANG.** Under `zh-Hans` the generic is *already* a Chinese face, so a tail
written **after** it is never consulted. Proven on this wave's tracer with
`CSS.getPlatformFontsForNode`:

| stack | Han renders in |
|---|---|
| `Garamond, 'Times New Roman', serif, 'PingFang SC', …` | **Songti SC** — the tail is dead code |
| `Garamond, 'Times New Roman', 'PingFang SC', …, serif` | **PingFang SC** — the tail is what the stack says |

Three of the four already-shipped zh plugins (O-Octagon, O-Bitrot,
O-MicrotonalSampler) use the correct before-the-generic form. **Only the
O-Chorus pilot does not**, so its tail is inert: its Chinese renders in a serif
face, not the sans it names, and its geometry pins were measured against Songti
SC. It looks right on macOS *because* the generic supplies a CJK face — on a
host where it does not, the tail would finally be reached and every metric would
change. **Not fixed here** (it would move O-Chorus's geometry and re-open its
pins); recorded for whoever owns that plugin next.

## 2. THE SAME MECHANISM BREAKS *LATIN*, AND THAT IS THE CHEAPER HALF TO MISS

A stack whose only surviving family is the bare generic renders **Latin**
through a language-resolved face too. O-AnalogSaturation's `.vu-scale` read
`'Garamond', serif`; Garamond is not a macOS face, so its ASCII dB ticks
rendered **Times** under en/fr and **Songti SC** under zh. All six ticks shrank,
the `space-between` row redistributed, and assertion 7 named **twelve movers —
not one of them holding a translated string.** O-Bass carried the identical
shape on `.preset-nav-btn` and `.preset-name`.

**Grep every wave plugin for a `font-family` whose non-generic families are all
absent on the build machine.** The fix is to NAME the face (`'Times New Roman'`)
— no CJK tail, because those nodes hold no Han. It costs one line and it is
invisible to every gate until a third language arrives.

## 3. THE TWO-LANGUAGE GATE-FILE CENSUS UNDER-COUNTS — 12 was really 14

The Stage-3 inventory greps for `en,fr` or `['en', 'fr']`. **Four plugins in
this wave were two-language and only two of them matched:**

| plugin | how it named its languages | on the census? |
|---|---|---|
| O-Comp | `LANGUAGES.join(',') === 'en,fr'` | yes |
| O-Texture | `LANGUAGES.join(',') === 'en,fr'` | yes |
| **O-Prism** | `for (const lang of ['en', 'fr', 'en'])` | **no** — three elements, the regex wants `'fr'` then `]` |
| **O-AnalogSaturation** | `driveLanguage('fr', …)` | **no** — no array literal at all |

**A gate that names its languages anywhere cannot see a third one arrive,
whichever syntax it uses.** The remaining ten on the census are still ten, but
wave 4b should assume its plugins are two-language until proven otherwise —
read the file, do not trust the grep.

All four were repaired the same way: **derive from the table's own `LANGUAGES`
export, and ABORT rather than fall back.** The derive-or-abort control was fired
on every one (planted empty `LANGUAGES` → exit 2, reverted by targeted edit,
i18n.js digest byte-identical each time).

## 4. A DIRECTION-SPECIFIC ASSERTION IS VACUOUS AGAINST HALF THE LANGUAGES

O-Prism's tip gate asserted **"French GROWS at least one tip's height"**.
Measured on that page: English 123.9 px, **French 139.3 (grows), Chinese 108.5
(SHRINKS)**. The assertion would have hard-failed on the zh arm — not because
anything was wrong, but because it tested a direction.

Rewritten to assert a **DIFFERENCE**: what the gate actually needs is that the
non-English pass *measured something else*. If no tip's height differs from
English, that pass is the same measurement twice and its half of the clamp
assertion is decoration. **Wave 4b: any assertion phrased "grew" or "shrank" is
half-blind. Assert change, or assert equality.**

## 5. CANVAS `fillText` CARRIES PAGE COPY THAT NO CSS RULE AND NO GATE REACHES

O-Comp draws two captions on its envelope display with `envCtx.font = '11px
Garamond, serif'` + `fillText`. **Canvas text is not in the DOM**: no stylesheet
reaches it, `check-ui-labels` cannot measure it, `boot-all-uis` does not see it,
and a missing CJK face there ships as a silent fallback — or as tofu — that
**nothing in this repo can detect**. The font string now carries the tail
explicitly.

**Wave 4b: `grep -l 'ctx.font\|\.font *=' plugins/*/Source/ui/public/index.html`
before starting.** One of six carried it here.

## 6. THE VISIBILITY FLAG MUST BE **OR**'d ACROSS STATES

Measuring the font stacks by walking `tests/i18n-states.json` cumulatively and
keying each node on **first sighting** makes every node read as *invisible* for
any panel the walk later navigates away from — the same blind spot as sweeping
only the resting page, arriving one step later.

On O-Prism the first sweep found **19** visible-Han nodes on the shared button
stack; OR-ing visibility across states found **48**. Twenty-nine nodes — the
whole wavetable editor, the tuning file buttons, the generator — would have
shipped untailed, **and no gate would have said so**, because a missing tail is
a font fallback, not a geometry change.

## 7. FOUR GEOMETRY SHAPES, AND ONLY ONE IS A LINE HEIGHT

Across 58 fixed movers in this wave:

**(a) `line-height: normal` inheritance** — still the dominant cause (64 of 76
node shapes on O-Prism). Pin each named leaf at its **measured English line
box**, derived from the BOX (height − padding − border), unitless, never global.
*A pin can leak DOWN into a differently-sized child*: O-Prism's `.tk-interval`
span is 15/12 and the `<strong>` inside it 14/12, so one pin on the parent would
have moved the **English** arm.

**(b) A content-sized element that SHRANK.** O-Prism's subtitle lost 109 px and,
as the third of four `space-between` children, moved the preset browser
**36.34 px in all 24 states**. **The file's own comment recorded a French-era
`width` pin REVERTED there** because French ran only 1.31 px narrow and that
landed under tolerance — *"a pin whose negative control passes is decoration"*.
That reasoning was correct on the evidence it had and is **wrong with a third
language in the file**. Expect to re-open French-era decisions, not just
French-era pins.

**(c) HAN WRAPS BETWEEN CHARACTERS, so a Han run's min-content width is ONE
character.** Hit twice: O-Prism's auto-sized rotation table squeezed 模式 to
72.47 px against Mode's 82.02 *and redistributed the difference across ten
columns*; O-Freeze's LFO shape button squeezed 正弦 **narrower than the "Sin" it
replaced AND wrapped it to two lines**. `nowrap` **plus** a min-width — nowrap
alone stops the wrap but leaves the box narrower.

**(d) NEW — LATIN DISPLAY CONVENTIONS APPLIED TO HAN.** `letter-spacing` and
generous side `padding` exist to give a short Latin caption presence. Neither
survives translation: `text-transform` is a no-op on Han and 1.2 px between
ideographs is not a Chinese convention, it is 6 px of extra box. Three sites
this wave (O-Prism's footer caption and Osc A/B toggle, O-Freeze's shape
buttons). Fixed under **`html[lang="zh-Hans"]`**, which cannot reach the en or
fr arm at all.

> **And the trim value must be MEASURED, not calculated.** On O-Freeze the
> figure derived from the glyph advance (5.9 px) left the button 2.08 px too
> wide, because the Latin letter-spacing applies *between the ideographs too*.
> 4.86 px is what the measurement returned.

> This is *not* the `:root:lang()` shape O-Octagon's stylesheet rules out — that
> note is about `font-family`, where 29 element-level declarations would outrank
> a `:root` rule. `letter-spacing` and `padding` here have no competing
> declaration to lose to.

## 8. `--emit <Plugin>` DOES NOT SCOPE THE BATCH — pass `--plugin` too

The tool's own usage line reads
`--emit O-Chorus --out /tmp/b.tsv`. Following it emitted **785 rows — the entire
corpus**, 758 of them already-shipped `bt` rows from Stages 2–3, which would
have been dispatched to an external reader as this plugin's work. `emit()` reads
the `--emit` argument, records it in the manifest as `target`, **and filters
nothing**; the row filter is `--plugin`, which `pluginList()` reads. R5 said
"table-scoped"; it is **corpus-scoped**. Pass **both**.

## 9. THE OPTION-WORD RULE CUTS BOTH WAYS — and the draft got it backwards 17 times

- A word that is an **`AudioParameterChoice` option string** is `I18N_EXEMPT`
  and **stays English in every language**, because it is what the button says.
- A word that is a **localized caption** must be **in Chinese**, or the body
  sends a reader looking for a control that is not on the page.

Both errors were present on O-Prism: `Serial`/`Parallel` had been translated to
串联/并联, while the LFO sync, free-run and bypass captions had been left in
English where the buttons read 自由 / 同步 / 重触发 / 自由运行 / 开 / 关.

**The discriminator is `I18N_EXEMPT` membership and it is mechanical.** A scan
requiring every exempt option word in an English body to survive verbatim into
the Chinese finds this class in seconds. **Run it per plugin.** (`Sync` is both
an exempt option string *and* `ui.sync`, a localized caption — the scan flags
it and a human resolves it.)

**Units in a range clause follow the READOUT.** O-Prism's knob prints ` st` and
` ct`, so five range clauses that read 半音 / 音分 now read st / ct; the
surrounding prose still uses the words.

## 10. SWAPPING A LATIN TOKEN FOR A HAN ONE LEAVES A Z8 VIOLATION BEHIND

The Z4 space in `显示 ON，` is **correct** while the token is Latin. Replace the
token with its Chinese rendering and you get `显示 开，` — a plain U+0020 between
two Han characters, which is **Z8**. It happened **ten times in one edit** on
O-Prism.

Rule Z8 (added in 260904-q4j) caught all ten. Z4 cannot: the pair never enters
its Latin/Han census. **This is the first time Z8 caught a live defect, and it
caught one introduced in the same session that shipped it.** The zero afterwards
agrees with an independent standalone scan (0 = 0), which is what that rule's
own promotion criterion requires.

## 11. THE GLOSSARY-COLLISION SCREEN HAS A BLIND SPOT

The R3 screen pushes a plugin's English name strings through `TERMS` and reports
two different keys landing on one root. **It only fires when BOTH sides are
glossary keys.** On O-Prism, `Osc A` *is* a key and `Oscillator A` is not; both
render 振荡器 A and the screen was silent.

**Add the mechanical downstream check**: after authoring, group the zh renderings
and report any two *different* keys sharing one, excluding same-control pairs
(a caption and its own tooltip title) and same-English pairs. It found three
pairs on O-Prism (two long/short oscillator forms, one caption matching its own
title — all benign) and one on O-Bass, and **zero real collisions across all
six**.

The screen itself is not inert: run against O-Bitrot it names that page's three,
`jitter`/`dither` → 抖动 among them.

## 12. TWO GLOSSARY ROOTS ARE DEFECTIVE, AND THEY ARE NOT THE SAME KIND

| root | back-translations | verdict |
|---|---|---|
| `botanical` → **植物律** | r1 "Phytometric", r2 "Plant Law [or: Plant Rhythm]" | **RE-AUTHORED** 植物插画 + `termNote`; r3 returned "Plant illustration" |
| `Rank-2 Temperament` → **二阶音律** | "Second-order temperament" | **SHIPPED**, concern recorded |

**The difference is stateable.** 二阶音律 back-translates to a *defensible
specialist synonym* — a terminology preference with no page collision, which
Stage 3 already declined to override on one executor's judgement. 植物律 uses 律
(temperament / law) on the alt text of a **decorative plant illustration**, and
**two different models in independent sessions both failed to read it as the
English**. A row whose back-translation is not its source has not met the `'bt'`
bar, and promoting it would assert something untrue. **That is what `termNote`
is for: an evidenced defect, not a preference.**

**Neither root is edited in the glossary** — changing a settled root is a
repo-wide change that could put other plugins out of Z5 conformance. Both go to
the glossary owners.

## 13. THE FALSE-ENUMERATION DEFECT IS WORSE THAN STAGE 3 RECORDED — 12 for 6

Stage 3 found four plugins whose language body **counted the selector's
options**. This wave found that defect in all six *and a second one beside it in
all six*: **the gear tooltip enumerates what the settings panel holds**, and
every one of these plugins added a hover-help switch in a later version without
updating it.

O-AnalogSaturation's said *"there is no hover-help switch and no other
preference"* — **false since v1.4.0**, and its markup comment said the same
thing thirty lines above the switch itself. O-Prism's said *"It holds one
control"* — false since v1.23.0, four versions.

**Twelve false sentences across six plugins. All removed, never extended, in
English and French both** — deletions only, so the French review flags stand.
No gate can see any of it: the sentences stay grammatical and the tooltips
render. **Wave 4b should assume both defects are present and check both.**

---

## Deviations from plan

### [Rule 1 — Bug] Twelve false hover-help sentences across six plugins
Finding 13. Six stale language enumerations (the known Stage-3 defect) and six
stale settings-panel enumerations (new). Removed in en and fr.

### [Rule 1 — Bug] Two font stacks rendered Latin through a language-resolved generic
Finding 2. O-AnalogSaturation `.vu-scale` (12 movers) and O-Bass
`.preset-nav-btn` / `.preset-name`. Neither holds Han; both are fixed by naming
the face.

### [Rule 2 — Missing critical functionality] Four gate files could not see a third language
Finding 3. Two were on the census; **two were not**. All four now derive their
language list with a fired derive-or-abort control. O-Prism's also had its
direction-specific assertion rewritten (finding 4), without which it would have
hard-failed on the zh arm.

### [Rule 2 — Missing critical functionality] Canvas text had no CJK face
Finding 5. O-Comp's two `fillText` captions.

### [Rule 1 — Bug] 26 rows re-authored after the reverse reads
17 option-word errors and 5 unit tokens on O-Prism, 3 sites on
O-AnalogSaturation (泛音 → 谐波), 1 on O-Texture (XY 板 → XY 控制板), 1 on
O-Bass (植物律 → 植物插画). Plus ten Z8 violations introduced by the O-Prism
correction itself and caught by the lint (finding 10).

### [Scope] `scripts/i18n-zh-glossary.js` untouched
Finding 12. Two roots are reported as defective rather than edited; editing a
settled root is a repo-wide change this task does not own.

### [Scope] `scripts/i18n-zh-lint.js` untouched — the gate flip is NOT made here
The lint is at **0 findings across all 43 plugins and 10/10 on its self-test**,
so the promotion criterion for `exit 2` is met. The flip is deliberately left to
whoever owns the Stage-4 gate decision, exactly as 260904-q4j left it.

### [Scope] O-Chorus's inert CJK tail not repaired
Finding 1. Fixing it would move O-Chorus's geometry and re-open its pins, which
is its own pass.

### [Out of scope] 19 late tip bindings on O-Bells and O-IntonationPad
Pre-existing, neither in this wave, `DEAD 0` / `failed 0` throughout. Logged in
`deferred-items.md` rather than fixed.

---

## Gate results — final, against the shipped tree

| Gate | Result |
|---|---|
| `check-i18n` (repo-wide) | **exit 0** — `ALL CHECKS PASS — 43 localized plugin(s)` |
| `check-i18n --plugin` ×6 | **exit 0** each; `LANGUAGES` reads three |
| `check-ui-labels` O-AnalogSaturation | **exit 0**, 87 PASS, **0 FAIL** |
| `check-ui-labels` O-Prism | **exit 0**, **1011 PASS, 0 FAIL** across 23 states — was 33 FAIL |
| `check-ui-labels` O-Comp / O-Freeze / O-Texture / O-Bass | **exit 0**; 171 / 87 / 87 / 129 PASS, **0 FAIL** |
| — en and fr arms | **0 FAIL on every plugin, before and after every pin** |
| `ui_tip_render_check` ×4 repaired | **exit 0**; real zh arms (O-Prism 2851 assertions passed) |
| — derive-or-abort control | **FIRED ×4** — planted empty `LANGUAGES` exits 2, reverted by targeted edit, digest byte-identical each time |
| two-language literal in the 4 repaired gates | **0**, comments included |
| repo-wide two-language census | **12 → 10** |
| `i18n-zh-lint` per plugin ×6 | **0 findings** on all ten rules; `BELOW SHIP BAR 0` |
| `i18n-zh-lint` repo-wide | **0 findings**, 1034 entries, **0 / 43 plugins** |
| `i18n-zh-lint --self-test` | **10/10** |
| Han-space-Han independent scan | **0**, agreeing with the lint's Z8 column |
| `i18n-fr-lint` | **exit 0**, `CLEAN`, **0 / 43** — French untouched throughout |
| `boot-all-uis --strict-tips` | **exit 0**, **0 DEAD**, 0 failed, 0 warn |
| `i18n-zh-backtranslate` (stage view) | **1338 rows / 1338 `bt` / 0 `mt`** |
| Han under each `Source/**/*.{h,cpp}` | **NO OUTPUT ×6**, each with its **positive control FIRED** on that plugin's own `i18n.js` |
| `PluginProcessor.h` codec | exactly **2** code occurrences of the tag each (comment-stripped) |
| stale language enumeration ×6 | **0** |
| `PLUGINS.md` | six rows **AGREE** with their CMakeLists; duplicate check **EMPTY** |
| blinding controls (C1–C5) | **FIRED on all 13 batches** — ids pure 12-hex, no key fragment, ids returned identical and in order, rows well-formed with no Han surviving, emitted table byte-identical to HEAD |
| fresh salt per round | **0 shared ids** between any two rounds of any plugin |
| `auval -v` ×6 | **AU VALIDATION SUCCEEDED ×6**, one cold rescan |

**Every negative gate has a positive control that fired.** A zero from a gate
whose control was never run is not evidence.

---

## Carry-forward for wave 4b

Wave 4b is **O-IntonationPad** (276 rows), **O-DigiDelay**, **O-AnalogEQ**,
**O-Tremolo**, **O-SimpleReverb**, **O-Emulator** — all `Source/ui/public/`.
O-Emulator and O-SimpleReverb are on the two-language census; **assume the other
four are two-language as well and read their gate files** (finding 3).

Run these **before** authoring:

1. `grep -l '\.font *=' plugins/<p>/Source/ui/public/index.html` — canvas text
   (finding 5).
2. Grep every `font-family` for a stack whose only non-generic families are
   absent on the build machine (finding 2).
3. The R3 glossary screen **and** the mechanical zh-collision check (finding 11).
4. Read both hover-help bodies for enumerations — the language one *and* the
   gear one (finding 13).

And while authoring: tail **before** the generic (1); OR the visibility flag
across states (6); check `I18N_EXEMPT` membership both ways (9); expect the Z8
space after any Latin→Han token swap (10); pass **both** `--emit` and `--plugin`
(8).

**O-IntonationPad already reports 17 late tip bindings** in `boot-all-uis` —
pre-existing, logged in this task's `deferred-items.md`, and worth resolving as
part of that plugin's wave rather than inherited silently.

---

## Commits

Seventeen, all path-scoped; the submodule guard ran clean before every one.
`.gsd/dispatch-isolation-sentinel.json` was modified before this work began and
was deliberately **never** staged.

| Commit | Scope | Content |
|---|---|---|
| `3e95ffc6` | `plugins/O-AnalogSaturation` | 20 entries at `'mt'`, tail before the generic, `.vu-scale` repaired, tip gate derived |
| `4bcec08c` | `plugins/O-AnalogSaturation` | 谐波 for harmonic, after the 27-triple read |
| `da1168a7` | `plugins/O-AnalogSaturation` | v1.5.0, promotion, CHANGELOG |
| `a4a21d17` | `plugins/O-Prism` | 267 entries at `'mt'`, 18 stacks, 4 geometry shapes, tip gate derived |
| `c6044e53` | `plugins/O-Prism` | 24 rows re-authored; option words both ways, units, ten Z8 |
| `e88ec412` | `plugins/O-Prism` | v1.24.0, promotion, CHANGELOG |
| `0785755e` | `plugins/O-Comp` | 39 entries at `'mt'`, canvas font tailed, tip gate derived |
| `5a3f8ab3` | `plugins/O-Freeze` | 36 entries at `'mt'`, the Han-wrap fix |
| `bfe6af10` | `plugins/O-Bass` | 29 entries at `'mt'`, two bare-generic stacks repaired |
| `1d48fe87` | `plugins/O-Texture` | 31 entries at `'mt'`, external stylesheet, tip gate derived |
| `d08672b6` | `plugins/O-Texture` | XY 控制板 after the 43-triple read |
| `55857b7c` | `plugins/O-Bass` | alt.botanical re-authored, root failed two reads |
| `bbd2208e` | `plugins/O-Comp` | v1.9.0, promotion, CHANGELOG |
| `698ea2bd` | `plugins/O-Freeze` | v2.5.0, promotion, CHANGELOG |
| `462d0991` | `plugins/O-Texture` | v0.5.0, promotion, CHANGELOG |
| `7ef20e1f` | `plugins/O-Bass` | v1.7.0, promotion, CHANGELOG |
| `051b9bcc` | `PLUGINS.md` | six rows corrected from the CMakeLists values |

## Known Stubs

**None.** No hardcoded empty value, placeholder string or unwired component was
introduced. Every zh row is authored, read back twice (three times on O-Bass)
and promoted; the two glossary roots reported as defective are *shipped
renderings with recorded reasoning*, not stubs.


---

## Self-Check: PASSED

All 18 claimed source files verified present on disk, plus this summary and
`deferred-items.md`. All 17 commits verified in `git log`. All six CHANGELOG
entries verified present at their claimed versions. Installed bundles verified
via `CFBundleShortVersionString`: O-AnalogSaturation **1.5.0**, O-Prism
**1.24.0**, O-Comp **1.9.0**, O-Freeze **2.5.0**, O-Texture **0.5.0**, O-Bass
**1.7.0** — VST3 and AU each, `-dev` variants only, no alternate-variant orphan,
and O-Texture's `Contents/Frameworks` carries `libonnxruntime.1.19.2.dylib` and
`libonnxruntime.dylib`.

The working tree carries no uncommitted plugin change. The only modified path is
`.gsd/dispatch-isolation-sentinel.json`, which predates this work and was never
staged.
