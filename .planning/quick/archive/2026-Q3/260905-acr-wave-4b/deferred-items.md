# Deferred items — quick task 260905-acr (Stage 4 wave 4b)

Out-of-scope discoveries from wave 4b, and the carry-forward for wave 4c.

---

## Deferred, with the evidence

### D1. `reviewed: 'native'` is OPEN on all six, and on all sixteen

Not a blocker and not a defect — a disclosed quality level. This project has no
native Chinese reader, so the ship bar for the whole rollout is `'bt'`: an
independent pass that has never seen the English renders the shipped Chinese
back into English and the drift is read in a language this project can read.
`i18n-zh-lint` rule R1 prints the open count on every run, and all six CHANGELOGs
state it in words. Closing it needs a native reader, which is a hiring decision
rather than a task.

### D2. Two glossary roots have now failed a reverse read, and neither is edited here

Both are REPORTED to the glossary owners rather than changed, because a settled
root is repo-wide: editing one puts every other plugin out of Z5 conformance in
the same commit. Each row carries a `termNote` with its evidence instead.

| root | English | what the reverse read returned | rounds / models | replacement |
|---|---|---|---|---|
| `植物律` | Botanical | "Phytometric", "Plant Law" | 2 models, 2 sessions (O-Bass, wave 4a) | `植物插画` |
| `延展` | Stretch | "Sustain", "Spread" | round 1 (O-IntonationPad) | `拉伸` |
| `时值` | Timing | "Duration", "Duration" | 2 models, 2 sessions (O-IntonationPad) | `时序` |

`植物律` is worse on O-IntonationPad than anywhere it has appeared before: `律`
is the exact character that plugin uses for a tuning system (`历史音律`,
`世界音律`, `二阶音律`), so the root reads as a *temperament* on the one page
where temperaments are the subject.

**A glossary pass that re-derives these three roots is its own task.** It has to
touch every plugin that renders them and re-run Z5 across the corpus.

### D3. O-Chorus's inert CJK tail (carried from wave 4a, still open)

Six declarations in the suite read
`Garamond, 'Times New Roman', serif, 'PingFang SC', 'Microsoft YaHei', sans-serif`
— the tail written AFTER the trailing generic, where Chromium never consults it.
Measured again during this wave and still present. Not wave 4b's plugins; it is
O-Chorus's own pass. The page still looks right on macOS, which is why it has
survived: the document-language generic supplies a Chinese face.

### D4. O-Bells' 2 late tip bindings

Left deliberately, and used throughout this wave as the CENSUS CONTROL: every
`boot-all-uis --strict-tips` run in this task was checked to still report
O-Bells at 2, because a run that reported 0 for O-Bells would mean the census had
gone blind rather than that O-IntonationPad's 17 had been fixed. Fixing O-Bells
would remove that control, so whoever does it should establish a replacement
control first.

### D5. `Z6` is inert on 549 of 552 glossary terms

Three terms carry a measured character budget; the rest are unbudgeted and Z6
cannot fire on them. The lint discloses this on every run rather than staying
silent, which is correct, but it means the width half of the Chinese review is
carried entirely by `check-ui-labels`' zh arm. Filling the budgets from
measured `check-ui-labels` output is a standing item across Stages 2–4.

### D6. O-IntonationPad has no `tests/ui_tip_render_check.js`

The largest table in the wave — 276 rows, 19 states, 80 tip bindings — is the
one plugin with no tip-render gate. Its tips are covered by `boot-all-uis` (a
report) and `check-ui-labels` (geometry only). Nothing was repaired there because
there is nothing to repair; writing one is a separate pass with its own budget.
Note that wave 4b's tip fix on that plugin was verified with a purpose-built
harness rather than a gate, so the verification is not repeatable by CI.

### D7. The measurement harness lives in scratch, not in the repo

Four defect classes in this wave were findable ONLY by computed style across
every state at the shipping frame — the SVG attribute form, the undeclared
`<button>`, the `line-height: normal` leaves, and the plate caption's wrap count.
The harness that found them was written for this task and is not committed. Each
wave rebuilding it is waste, and worse, each wave rebuilding it can rebuild it
wrong (this one did, twice: a non-cumulative state walk and a 1200 px viewport,
both of which hid real defects). **Promoting it to `scripts/` is the highest-value
carry-forward in this list.**

---

## Carry-forward for wave 4c — structural, in the shape wave 4a used

### C1. The flipped lint DID fire on a wave-4b authoring round. Once, correctly.

The constraint asked whether the flip would block real work. It did, exactly
once: `拉伸` departs from a settled glossary root, so Z5 exited 2 — and it fired
on the tooltip TITLE while the caption's `termNote` already exempted the caption.
**A tip title is lint-checked separately from the label it mirrors**, so an
exemption has to be stated on both or the pair goes out of sync the day one is
edited. Under the old report-only tool that would have been a line of output
nobody had to read.

Authoring itself was never blocked: 492 rows were drafted at `reviewed: 'mt'`
under the live gate with the lint exiting 0 throughout, which is the routing the
flip's own commit verified in source rather than assumed.

### C2. THE FONT-CARRIER CENSUS IS FOUR FORMS, NOT THREE

Wave 4b's plan named three carriers and measured a fourth. Run all four screens
on the next six:

| # | carrier | how to find it | seen on |
|---|---|---|---|
| 1 | CSS `font-family:` declaration | grep, property-and-colon | all six |
| 2 | SVG `font-family=` presentation attribute | grep, property-and-EQUALS | O-AnalogEQ (24) |
| 3 | custom property named for the TYPEFACE ROLE | grep `--serif`/`--sans`/`--mono`, **not** the word "font" | O-Emulator (1 edit → 8 decls) |
| 4 | **an element with NO declaration at all** | **computed style only** | O-IntonationPad (13), O-AnalogEQ (1) |

Form 4 is the new one. `<button>`, `<select>` and `<input>` do **not inherit
`font-family`** — the UA stylesheet gives them one, on this build Arial. A sweep
that reads every declaration AND every attribute in both files still misses them.
Repair by naming the tail with **Arial kept first**, so Latin metrics on the en
and fr arms do not move.

**The only reliable census is `getComputedStyle` over every visible node, in
every state, at the shipping frame.** Three of the four forms are greppable and
one is not, so a grep-based census is structurally incomplete.

### C3. Measure at the SHIPPING FRAME, and walk the states CUMULATIVELY

Both were got wrong in this wave and both hid real defects:

- **A wide viewport hides wrap-count defects.** O-Emulator's engraved plate wraps
  to three lines in en/fr and two in zh at the shipping 700 × 380 — and to one
  line in all three at 1200 px wide, where the defect does not exist. Parse the
  frame from `PluginEditor.cpp`, exactly as `check-ui-labels` does.
- **`tests/i18n-states.json` is a CUMULATIVE walk, not a set of one-shot clicks.**
  O-IntonationPad's rotation view (state 9) needs the tuning tab (state 3) first.
  Resetting between states measures a page those states never produce.

And W2 still holds: **OR the visibility flag across states.**

### C4. A gate can spell its language count in a form no census greps for

Wave 4a found two spellings; wave 4b found a third, and only by running the gate:

```js
if (drivenStates.length <= 2) {      // O-AnalogEQ — the literal 2 IS the language count
```

With a third language the zh pass recorded no heights at all and assertion 5
failed reporting that *nothing had moved*, on a page where five things had.
**Read every gate file end to end. Any integer literal near a language walk is a
suspect, not just a string literal.** The census is not the inventory: 3 of 5
gate files in this wave were off-census, against 2 of 4 in wave 4a.

### C5. Rewrite a direction-specific assertion as a DIFFERENCE

Three of five gate files required the non-English pass to be strictly TALLER.
True for French; false for Chinese, which shrinks. Measured this wave:

| plugin | shape |
|---|---|
| O-Tremolo | `#depthKnob` 98 → 82, `#tempoButton` 114 → 98 |
| O-DigiDelay | SPREAD/WET/DRY 92 → 77 |
| O-AnalogEQ | 0 grew, 5 SHRANK |

What the assertion is actually for is catching a pass that measured the same
boxes as English. `Math.abs(h[s] - hEn[s]) > 0.5`, per non-English language off
the derived list.

### C6. DERIVE-OR-ABORT on every repaired gate, and fire the control

A derived language list can go empty, and an empty list walks zero languages and
reports every assertion green — worse than the hard-coded list it replaces. All
five repaired gates in this wave carry the guard, and the control was fired on
each: a planted `LANGUAGES = []` exits non-zero, and the reverting edit leaves
the file byte-identical (checked by sha256, never by `git checkout --`).

Note the two failure modes differ: the **array-literal** form goes blind, and the
**joined-pair** form (`LANGUAGES.join(',') === 'en,fr'`) HARD-FAILS on a correct
table. The second is louder but no less wrong.

### C7. Author the new language from the CORRECTED English

The wave's cheapest lesson. On O-SimpleReverb, one commit both deleted the gear
body's exclusivity clause from en and fr AND authored the zh row for that body —
from the text as it stood *before* the deletion. The Chinese went on asserting
what the English had stopped asserting, and **every gate was green**: `check-i18n`
only asks that the key resolves, the lint only checks typography and
terminology, and **no tool in this repo compares a `zh` body against its own
`en`**. Only the reverse read caught it, returning the deleted sentence.

**Ordering that makes it safe:** make the enumeration deletions first, re-read
the file, then author from the file as it then stands.

### C8. Do not spell the superseded literal in the comment that explains it

Known for gate files; it applies to TABLE comments for the same mechanical
reason. O-Tremolo's comment explaining the deleted enumeration quoted the deleted
sentence verbatim, so the wave's own stale-enumeration probe kept reporting a
plugin that had already been fixed. Caught by the tracer feedback gate re-running
the verify block, not by the authoring pass.

### C9. Two-line predictions to test on the next six

- **N10 generalised:** five of six gear bodies were false and one was correct.
  *Check both*, never assume both. O-IntonationPad's named both its choices and
  was deliberately left alone — recorded as a checked non-defect.
- **Preconditions worth re-measuring rather than carrying:** the plan's N7
  asserted that a late tip binding means the tip never carries its Chinese. It is
  FALSE on O-IntonationPad — all 80 anchors carried a tip at settle in all three
  languages, measured before any change. The ordering defect was real and worth
  fixing; the stated consequence was not. **Measure the consequence before
  budgeting the fix.**

### C10. The reverse-read dispatch shape that worked

```
claude -p "<prompt>" --model <model> --allowed-tools ""     # cwd OUTSIDE the repo
```

- `--allowed-tools ""` plus a cwd outside the repo plus an explicit prohibition
  in the prompt. Three independent barriers, because several of these plugins
  name themselves in their own copy.
- **`--forward-provenance` is REQUIRED at emit.** Without it `--ingest` refuses
  rather than reporting, and correctly so — it cannot otherwise tell an
  independent reverse pass from the pass that produced the Chinese. That refusal
  FIRED in this wave, which is worth recording: memory has it running inert once
  before.
- **Batch large tables in chunks of ~92 rows.** 276 in one dispatch is a long
  single response; three chunks returned cleanly, in order, every time.
- **A correction round needs a fresh salt AND a different model.** A fresh
  session of the same model guards correlation but not self-agreement. The
  fresh-salt control is cheap and worth firing: round 3's blinded ids shared zero
  with rounds 1 and 2.

### C11. zsh traps that fired during this wave

- **`set -- $var` does not word-split in zsh.** The auval sweep loop silently
  produced six empty verdicts before this was caught. Pass arguments explicitly.
- **`timeout` does not exist on this machine.**
- **Quote every `--include` glob** — unquoted, zsh expands it against the current
  directory, the grep fails entirely, and the count reads as a clean zero.
