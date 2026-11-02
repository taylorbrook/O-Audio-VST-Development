---
quick_id: 260903-rjm
type: execute
mode: quick
status: complete
date: 2026-09-03
tasks_completed: 3
tasks_total: 3
plugins_touched: 23
plugins_built_and_installed: 23
plugins_auval_pass: 23
commits:
  - 69918256  # Task 1 — O-Chorus tracer
  - 0e406c7b  # Task 2 batch 1
  - d4fd9f35  # Task 2 batch 2
  - 566a7fa6  # Task 2 batch 3
  - 7b3b689c  # Task 2 batch 4
  - a4bc791a  # Task 2 batch 5
  - 6ca98461  # Task 3 — check-i18n assertion [16]
files_changed: 120
actuals:
  tokens: 96000
  tasks: 3
  commits: 7
---

# Quick task 260903-rjm — a hover-help switch in the 23 plugins that had none

O-Chorus had language options but no way to turn the hover help off. Twenty of
the suite's forty-three plugins already had that switch; twenty-three did not.
All twenty-three have it now, and `check-i18n` assertion [16] makes "a language
selector without a hover-help switch" a red gate.

**All three tasks finished.** Every plugin is version-bumped, CHANGELOG'd,
rebuilt, reinstalled and `auval`-clean. Every gate the plan named was run and
its exit code recorded below. Nothing was skipped silently.

---

## Per-plugin table

`gates` are, in order: **CI** `check-i18n` · **UL** `check-ui-labels` · **BU**
`boot-all-uis --strict-tips` · **FR** `i18n-fr-lint` · **TR**
`ui_tip_render_check` · **SW** the cross-plugin functional switch probe (17
assertions). All values are exit codes; `n/a` means the file does not exist for
that plugin. `auval` is `auval -v <type> <CODE> OuDv`.

| # | Plugin | version | build | auval | CI | UL | BU | FR | TR | SW | commit |
|---|--------|---------|-------|-------|----|----|----|----|----|----|--------|
| 1 | **O-Chorus** | 1.5.0 → **1.6.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 (345 passed) | 0 | `69918256` |
| 2 | O-AnalogEQ | 1.3.1 → **1.4.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `0e406c7b` |
| 3 | O-AnalogSaturation | 1.3.1 → **1.4.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `0e406c7b` |
| 4 | O-Bass | 1.5.1 → **1.6.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `0e406c7b` |
| 5 | O-DigiDelay | 1.4.1 → **1.5.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `0e406c7b` |
| 6 | O-Tremolo | 1.8.2 → **1.9.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `0e406c7b` |
| 7 | O-Comp | 1.7.1 → **1.8.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `d4fd9f35` |
| 8 | O-Detune | 1.7.1 → **1.8.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `d4fd9f35` |
| 9 | O-Emulator | 1.2.1 → **1.3.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `d4fd9f35` |
| 10 | O-Freeze | 2.3.0 → **2.4.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `d4fd9f35` |
| 11 | O-SimpleReverb | 1.7.2 → **1.8.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `d4fd9f35` |
| 12 | O-Bassoon | 1.2.2 → **1.3.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `566a7fa6` |
| 13 | O-Bells | 4.3.2 → **4.4.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `566a7fa6` |
| 14 | O-Bowed | 1.6.2 → **1.7.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `566a7fa6` |
| 15 | O-Reed | 1.3.1 → **1.4.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `566a7fa6` |
| 16 | O-Wind | 1.18.2 → **1.19.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `566a7fa6` |
| 17 | O-Formant | 1.27.2 → **1.28.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `7b3b689c` |
| 18 | O-GrainScatter | 2.6.1 → **2.7.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `7b3b689c` |
| 19 | O-Texture | 0.3.1 → **0.4.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `7b3b689c` |
| 20 | O-TextureForge | 1.2.1 → **1.3.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `7b3b689c` |
| 21 | O-MicrotonalSampler | 1.25.2 → **1.26.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `7b3b689c` |
| 22 | O-Prism | 1.22.1 → **1.23.0** | ok | PASS | 0 | 0 | 0 | 0 | 0 | 0 | `a4bc791a` |
| 23 | O-ReverseDelay | 1.10.1 → **1.11.0** | ok | PASS | 0 | 0 | 0 | 0 | **n/a** | 0 | `a4bc791a` |

**O-Texture's plan version was wrong.** The scan table listed it at 1.19.2; the
`CMakeLists.txt` actually read **0.3.1**, so it went to 0.4.0. Bumping to the
number the plan named would have invented four minor versions that never shipped.

**The plan said O-GrainScatter was the only `app.js` renderer.** It is not.
Five of the twenty-three have the renderer outside `index.html` — O-Formant
(`js/main.js`), O-GrainScatter (`js/app.js`), O-Texture (`js/main.js`),
O-TextureForge (`js/i18n_init.js`) and O-MicrotonalSampler
(`js/sampler-app.js`) — and O-ReverseDelay's is `js/app.js` with a different
shape again (`showTooltip`/`hideTooltip`, module-level, not closures). Batch 4
was rebuilt around that measurement rather than around the table.

### auval type/code, for the record

`aufx`: O-AnalogEQ `OuAE`, O-AnalogSaturation `OaSa`, O-Bass `OBas`,
O-Chorus `OuCh`, O-Comp `OuCp`, O-Detune `OuDt`, O-DigiDelay `OuDD`,
O-Emulator `OEmu`, O-Formant `OuFm`, O-Freeze `OFCR`, O-GrainScatter `OuGS`,
O-Prism `OuPr`, O-ReverseDelay `ORvD`, O-SimpleReverb `OuSr`, O-Texture `OuTx`,
O-TextureForge `OuTF`, O-Tremolo `OuTr`.
`aumu`: O-Bassoon `OBsn`, O-Bells `OBls`, O-Bowed `OBwd`,
O-MicrotonalSampler `OMtS`, O-Reed `ORed`, O-Wind `OWnd`.

---

## Geometry, measured with each popover forced open

The plan's real risk was that a second row grows every settings popover. It
does — the question is whether it clips, and whether it grows *differently* in
French. Both were measured on every plugin rather than assumed. Every panel
below is **byte-identical in English and French**, and `check-ui-labels` [7]
reports **0 non-label elements displaced** on all 23.

| Plugin | popover rect (en ≡ fr) | frame | switch face en → fr |
|---|---|---|---|
| O-Chorus | y 40..94, 170 × 54 | 700 × 125 | 42.00 → 42.00 (also 开 in zh-Hans) |
| O-AnalogEQ | y 133.2..188, 170 × 54.8 | 920 × 220 | 42.00 → 42.00 |
| O-AnalogSaturation | y 40..104, 186 × 64 | 600 × 450 | 42.00 → 42.00 |
| O-Bass | y 226..283, 170 × 57 | 420 × 320 | 42.00 → 42.00 |
| O-DigiDelay | y 109.2..164, 170 × 54.8 | 700 × 196 | 42.00 → 42.00 |
| O-Tremolo | y 276..363, **120** × 87 | 600 × 400 | 98.00 → 98.00 |
| O-Comp | y 25..87, 176 × 62 | 620 × 360 | 42.00 → 42.00 |
| O-Detune | y 386..448, 176 × 62 | 600 × 480 | 42.00 → 43.61 |
| O-Emulator | y 322..383, 164 × 61 | 620 × 430 | 42.00 → 42.00 |
| O-Freeze | y 42..104, 184 × 62 | 550 × 530 | 42.00 → 43.61 |
| O-SimpleReverb | y 256..313, 170 × 57 | 500 × 350 | 42.00 → 42.00 |
| O-Bassoon | y 37..103.59, 190 × 66.59 | 900 × 600 | 42.00 → 45.30 |
| O-Bells | y 47..112, 200 × 65 | 800 × 600 | 42.00 → 42.00 |
| O-Bowed | y 42..109.19, 200 × 67.19 | 900 × 600 | 42.00 → 46.97 |
| O-Reed | y 37..103.59, 190 × 66.59 | 900 × 600 | 42.00 → 45.30 |
| O-Wind | y 39..105, 190 × 66 | 900 × 600 | 42.00 → 46.97 |
| O-Formant | y 34..117, 168 × 83 | 800 × 600 | 42.00 → 42.00 |
| O-GrainScatter | y 39..94, 170 × 55 | 900 × 800 | 42.00 → 42.00 |
| O-Texture | y 51.5..115.69, 196 × 64.19 | 800 × 600 | 42.00 → 46.97 |
| O-TextureForge | y 35.5..94.5, 190 × 59 | 900 × 600 | 42.00 → 44.25 |
| O-MicrotonalSampler | y 51.5..128.5, 208 × 77 | 900 × 640 | 42.00 → 50.66 |
| O-Prism | y 32..92, **168** × 60 | 1200 × 800 | 42.00 → 43.61 |
| O-ReverseDelay | y 54..141, 196 × 87 | 940 × 768 | 42.00 → 53.20 |

A switch face that grows in French is the `min-width: 42px` floor doing what it
is for: the row is `space-between` and the button is a `[data-i18n]` node, so
the growth goes **leftward** into slack the panel already had and the button's
right edge does not move.

### The two panels that were widened, and why English was measured too

Only two of twenty-three needed a layout change. In both the widening was
measured in **both** languages, not just the one that forced it — the half a
"make French fit" change usually skips.

**O-Tremolo, 116 px → 120 px.** This panel stacks its caption ABOVE its control,
so the caption gets the panel's whole content box. At 116 px that box was
94.00 px and the French *AIDE AU SURVOL* renders at **95.98 px** —
`check-ui-labels` [4] reported exactly that 1.98 px overflow. 120 px gives
98.00 px. English at the same width: *HOVER HELP* **71.95 px**, *LANGUAGE*
**63.55 px**; French *LANGUE* **47.11 px**. Still a HARD width, so the panel's
rectangle stays language-invariant by construction.

**O-Prism, a caption pin replaced by a panel pin.** Its popover was a single
flat flex row — caption and `<select>` as direct children, no `.settings-row`
wrapper anywhere on the page — with a `width: 59.77px` pin on `.settings-label`
because the panel is `right: 0` and a narrower caption moves its LEFT edge. That
pin only covers the captions it was measured against: the French *AIDE AU
SURVOL* renders at **92.45 px**, half again the 59.77 px English *LANGUAGE* box,
and would have overflowed it silently. The popover is a two-row column now with
a hard `width: 168px`, the caption pin is gone, and every caption sizes itself
honestly (*HOVER HELP* 70.53, *LANGUE* 44.80, all inside 168).

**O-Formant needed no widening — it needed reparenting.** Its popover is
`display: block` and its caption and `<select>` share ONE `.settings-row`, so a
row inserted after the `</select>` landed INSIDE that row. `check-ui-labels` [7]
reported four moved non-label elements (`#settings-popover` dx=−4.0 dw=+4.0,
`#lang-select` dx=−15.0) because a French caption in a space-between row pushed
the panel's own `min-width: 168px` box wider. Reparented as a sibling with
`.settings-row + .settings-row { margin-top: 6px }` — a `gap` on the parent does
nothing on a block container — [7] reports 0 moved.

---

## Task 3 — assertion [16] and its controls

`node scripts/check-i18n.js` exits **0 across all 43 plugins** with [16] live,
and reports `43 plugin(s) with a language selector were EXAMINED`.

The rule is three separate `check()` calls, not one conjunction — `a && b && c`
reports "failed" without saying which half, and the three failure modes want
different fixes. Each was proven to fire independently:

| control | what was broken | result |
|---|---|---|
| **NC-1** | `id="tips-toggle"` → `id="tips-toggle-BROKEN"` in O-Chorus's index.html | `FAIL: [16] … EXACTLY ONE hover-help switch … found 0`, **exit 1**. Checks 2 and 3 correctly did not run. |
| **NC-2** | the `['#tips-toggle', 'tip.tipsToggle']` TIP_BINDINGS row deleted | check 1 **PASS**, `FAIL: [16] #tips-toggle is named by a TIP_BINDINGS selector`, **exit 1** |
| **NC-3** | `'tip.tipsToggle'` renamed in I18N | checks 1 and 2 **PASS**, `FAIL: [16] the key #tips-toggle binds exists in I18N`, **exit 2** (assertion [2] also fires — expected) |
| **NC-4** (vacuity) | a fixture root whose only plugin has no `#lang-select` | `FAIL: [repo] [16] 0 plugin(s) … were EXAMINED — zero is not a legitimate zero here`, **exit 1** |

After every control, `git status --short plugins/O-Chorus` reported **clean** —
the file is byte-identical to its committed state.

Both id spellings are accepted. Eleven plugins are on `#help-toggle` and twelve
on `#tips-toggle`; a gate demanding one spelling would be a rename of eleven
shipped plugins disguised as a lint.

### The switch itself also has a negative control

The plan's recipe is a `show()` gate, and a gate that is green whether or not it
exists is decoration. Two proofs were run:

- **O-Chorus, in its committed regression gate.** `tests/ui_tip_render_check.js`
  gained a section 8 (16 assertions: default ON, both flip directions, the knob
  tip disappearing **and returning**, both `data-tip-always` controls surviving
  while `#lang-select` stays silent, the face localized in fr *and* zh-Hans in
  both states, and the localStorage write both ways). Deleting the single
  `if (!tipsEnabled && !el.hasAttribute('data-tip-always')) return;` line turns
  assertions [4] and [7] **red**; restoring it turns them green again — 345
  passed, 0 failed.
- **O-Bass, with the cross-plugin probe.** Same deletion, same result: 2 of 17
  red, green again on restore.

---

## Whole-suite verification (the plan's eight)

| # | check | result |
|---|---|---|
| 1 | `node scripts/check-i18n.js` | **exit 0**, 43/43, [16] live, 43 examined |
| 2 | `node scripts/boot-all-uis.js --strict-tips` | 43/43 clean, 0 warn, 0 failed, **0 DEAD bindings** |
| 3 | `node scripts/i18n-fr-lint.js` | **exit 0**, 0 of 43 plugins with findings |
| 4 | `node scripts/i18n-zh-lint.js --plugin O-Chorus` | **exit 0**, 0 findings across 9 rules |
| 5 | `check-ui-labels` on all 23 touched | **exit 0 on all 23** (77 would have printed; none did) |
| 6 | the MISSING-TOGGLE scan loop | prints only `scan done` — **43/43 have a switch** |
| 7 | version-bumped, CHANGELOG'd, rebuilt, reinstalled, auval-clean | **23/23** |
| 8 | `git log` path scoping | **7 commits, every path under `plugins/` or `scripts/check-i18n.js`.** No `git add -A`, no `git commit -a`, `git branch --show-current` and `git status --short` re-checked immediately before each. The submodule guard was run before every commit and never fired. |

---

## Deviations from the plan

1. **Tip key spelling follows each plugin's own convention, not a suite-wide
   literal.** The plan specified the bare key `'tips-toggle'`. Twenty-two of the
   twenty-three prefix every key in their file with `tip.`, so they got
   `tip.tipsToggle`; one mixed key in an otherwise uniform file is drift no lint
   can see. **O-ReverseDelay** is on the older bare convention throughout and
   got `tips-toggle`, matching its own `lang-select`. Assertion [16] checks that
   the bound key *exists*, so it is indifferent to the spelling.

2. **O-Chorus's `tip.tipsToggle` zh-Hans body ships at `reviewed: 'mt'`, and so
   does `tip.settings`'s.** `'bt'` — the rollout's ship bar — asserts that a
   second, INDEPENDENT pass, one that never saw the English, rendered the
   Chinese back and the drift was read. No such pass was available in this
   session (no independent AI CLI is installed; `codex`, `gemini`, `llm`,
   `ollama` all absent), and the session that wrote the two strings cannot be
   the session that blindly reverses them. The four short new Chinese
   strings — 悬停帮助 / 开 / 关 / 开关悬停帮助 — are settled glossary ROOTS taken
   verbatim from `scripts/i18n-zh-glossary.js` and stay at `'bt'`, exactly as
   the plan authorises. `i18n-zh-lint` reports `BELOW SHIP BAR … 2` and exits 0
   (it is still report-only). **Recorded rather than claimed** — see the
   *Deferred* section.

3. **`tip.settings`'s body was rewritten in O-Chorus.** It said the panel holds
   the interface language "and nothing else". It holds two rows now. Left alone
   it would have been a tooltip that contradicts the panel it describes, and
   nothing in the plan or the gates would have caught it.

4. **Six test harnesses were edited, five of them to replace a proxy or a
   byte-pin with the property it stood for.** None of these is a relaxation:
   - **O-Bells [1c] and O-Reed [1]** asserted `.settings-row is unique on this
     page` — a proxy for *closest() is right by construction, not by luck*. A
     second row makes the count wrong without making the binding wrong: the
     binding resolves `#lang-select` by its unique id and then walks
     **ancestors**, which cannot reach a sibling row however many exist. Both
     now assert directly and more strongly that the resolved wrapper contains
     `#lang-select` and does **not** contain `#tips-toggle`.
   - **O-Texture and O-TextureForge [0]** asserted the init line byte-for-byte
     (`try { initI18n(); setupTooltips(); } catch`), which goes red the moment a
     second guarded call is added. Each now asserts the ORDER, plus separately
     that `initializeTipsToggle()` is inside that same guarded block — the
     property that actually matters, since the toggle reads `setLabel` and a
     top-level call reaching a lower `let`/`const` is the TDZ throw that takes
     the whole module with it.
   - **Hard-coded chrome counts** updated with their explanations, not just
     their numbers: O-Bowed 30→31, O-Wind `TIP_COUNT` 52→53, O-Formant "ten
     non-`[data-param]` anchors"→eleven, O-GrainScatter "two chrome"→three,
     O-Prism 107→108.
   - **Chrome anchor lists** gained `'#tips-toggle'` in eight harnesses so the
     new binding is **hovered**, not merely counted; O-Bells' `POPOVER_SELS` and
     O-Prism's `POPOVER_ONLY` gained it too, since an anchor inside a
     `display: none` panel measures 0 × 0 and would be reported unhoverable.

5. **`ui.on` / `ui.off` were reused, never duplicated,** in O-Bells and
   O-SimpleReverb, as the plan required. O-SimpleReverb's are `ON`/`OFF` and
   `ACT.`/`DÉS.` — its own casing and its own shorter French, which its geometry
   was already measured against. The functional probe therefore reads its
   expected faces from each plugin's **own LABELS table**; a probe hard-coding
   `"On"` would have reported that plugin broken for having done the right
   thing, which it did on the first run before being fixed.

6. **No `--emit`/`--ingest` back-translate run.** The plan asked for a blind
   pass with `--manifest`. The tool refuses to report a triple whose reverse
   pass is the same pass that produced the Chinese, correctly, and there was no
   independent reverse pass to give it. Deviation 2 is the honest outcome.

---

## Deferred / open

- **Two O-Chorus zh-Hans bodies below the `'bt'` ship bar** —
  `tip.tipsToggle.b` (new prose) and `tip.settings.b` (rewritten). Queued for
  the next reverse batch: `node scripts/i18n-zh-backtranslate.js --emit
  O-Chorus`. Recorded in O-Chorus's CHANGELOG under *Known gap*.
- **O-ReverseDelay has no `tests/ui_tip_render_check.js`** — one of two of the
  forty-three without one. Its switch was verified with the cross-plugin
  functional probe (17/17) but that probe is not committed, so this plugin
  carries **no standing regression gate** for the switch. Recorded in its
  CHANGELOG under *Not covered*.
- **19 "late" tip bindings reported by `boot-all-uis --strict-tips`, across 2
  plugins** — O-IntonationPad (17) and O-Bells (2: `#ref-pitch-knob`,
  `#octave-stretch`). **Pre-existing and not caused by this work.**
  O-IntonationPad is not one of the 23. O-Bells' two anchors are byte-identical
  to the pre-change file (`git show e9aab4fc:…` differs only in line numbers)
  and are tab-gated controls that do not exist at first sweep. DEAD bindings —
  the arm that matters — are **0**, and `#tips-toggle` is not in the late list
  on any plugin.
- **The plan's Task 2 human-check was not performed by a human.** Spot-checking
  O-GrainScatter, O-Prism and O-Wind in Logic is a manual step; the equivalent
  behaviour was driven headlessly on all 23 by the functional probe (default ON,
  both flip directions, tips gone and returning, `data-tip-always` surviving,
  French rendering without clipping, localStorage persisting). O-Chorus's
  in-DAW check is likewise outstanding.

---

## Self-Check: PASSED

Files claimed and verified present:
`plugins/O-Chorus/Source/ui/public/index.html`,
`plugins/O-Chorus/Source/ui/public/js/i18n.js`,
`plugins/O-Chorus/tests/ui_tip_render_check.js`, `scripts/check-i18n.js`, and a
`#tips-toggle` in all 23 index.html files (the Task 2 scan loop prints no
MISSING lines).

Commits claimed and verified in `git log`: `69918256`, `0e406c7b`, `d4fd9f35`,
`566a7fa6`, `7b3b689c`, `a4bc791a`, `6ca98461` — 120 files changed,
4882 insertions, 131 deletions.
