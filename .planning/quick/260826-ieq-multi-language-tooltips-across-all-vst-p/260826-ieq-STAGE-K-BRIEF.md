# Stage K executor brief — T15: the 21 bare plugins, LABELS ONLY

Read this in full before touching a file. It is the standing brief for all 21
dispatches; the dispatch prompt names only the plugin and its batch.

## What this stage is, and what it is NOT

These 21 plugins have **no tooltip copy today** — only stray native `title=`
attributes. This stage localizes their **existing visible text**. It does
**NOT author hover-help prose**. That is Stage M, deferred by recommendation.

Nothing here can regress a tooltip, because there are none. `TIP_BINDINGS` is
`[]` and `I18N` holds only composed/JS-written strings (or is empty). The gate
was taught this shape in the commit immediately before Stage K's first plugin:
assertion 2 now reports "N tip(s) bound" and accepts 0 **only** when no `I18N`
entry carries a non-empty body.

## The authority documents, in order

1. `.planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-PLAN.md`
   - lines 195-299 — **CANONICAL CONTRACT V2**. Sections 1-8. Read every one.
   - lines 1088-1153 — **T15**, this task, with the batch tables.
   - lines 964-1030 — **T13**, whose ten per-plugin steps this stage reuses
     minus the tooltip-copy move.
2. `260826-ieq-SUMMARY.md` — the stage log. Read at minimum the
   **"Carried into Stage K"** section (search for that heading).
3. `scripts/i18n-canon.js` — the canon block held as DATA. `I18N_CANON_V2` is
   what your plugin's `app.js` must carry **byte-identical** after comment
   stripping and whitespace normalisation. Assertion 6 byte-compares it.

## The reference implementation

`plugins/O-Tapestop` is the label pattern-bearer. Read, in this order:
- `Source/ui/public/js/i18n.js` — table shape: `LANGUAGES`, `I18N`, `LABELS`,
  `I18N_EXEMPT`, `TIP_BINDINGS`. Note `LABELS` entries are `{en:{t}, fr:{t,
  reviewed}}` — one string, no body.
- `Source/ui/public/js/app.js` — where the canon block sits and how it is
  called from deferred init inside try/catch.
- `Source/PluginProcessor.h:154`, `PluginProcessor.cpp:947,1001-1004`,
  `PluginEditor.cpp:267-282` — the C++ language pair and persistence.
- Its `CMakeLists.txt` — `i18n.js` in `juce_add_binary_data` SOURCES **and** a
  `getResource()` branch. Assertion 8. A file embedded but not served, or
  served but not embedded, is a 404 that presents as a missing panel and
  nothing else — the highest-frequency mistake in this work.

Your plugin's gear popover is styled inside **its own** visual system. It is
not one uniform widget pasted in unchanged.

## The ten steps, per plugin

1. `node scripts/i18n-extract.js --plugin <Name>` — review **every** `UNSURE`
   and `READOUT` row by hand. The extractor's counts have been wrong about
   every plugin in every stage so far, in both directions.
2. `grep -rn 'setVisible' plugins/<Name>/Source/` — abort and report if it
   targets the web view. A hidden `WebBrowserComponent` drops native-function
   completions and the one-shot language pull never settles.
3. `grep -n -i version plugins/<Name>/CMakeLists.txt` — several declare the
   version through a `set(<PLUGIN>_VERSION ...)` variable consumed by both the
   plugin and its render harness. Bump the variable, never a second copy.
   **Also check for `PLUGIN_VERSION`** — it is not a JUCE keyword and is
   silently ignored; if you find it, REPORT it, do not fix it (it is a
   host-visible change pending a human decision across seven plugins).
4. Labels added; `data-i18n` applied; attributes keyed; **every native
   `title=` DELETED** per contract §4. Where a `title=` is an element's only
   help, its text becomes `data-i18n-aria` — **no new prose is invented**.
5. Canon v2 block, called from the plugin's existing deferred init inside
   try/catch.
6. Gear popover with the language selector, styled in the plugin's own system.
7. C++ language pair and persistence, mirroring whichever idiom that plugin
   already uses for its own non-parameter state. **A non-parameter bool/string
   on the APVTS state tree round-trips through XML as a STRING `var`** — the
   `isVoid()` guard, not `isBool()`.
8. `i18n.js` into `SOURCES` + a `getResource()` branch + the import, all in
   the same commit.
9. French drafted, every entry `reviewed: false`.
10. Both gates, then every gate in that plugin's own `tests/`, then
    build-and-install, then commit discipline (below), then `git show --stat`.

## The D-01 test — which strings localize, in three arms

1. Is the string an `AudioParameterChoice` option, **byte-identical**? Then it
   is EXEMPT — the page and the host automation lane must agree. Byte-identity
   is the test: `12-TET` matching `TUNING_MODE`'s option verbatim is exempt;
   `CUSTOM` against an option spelled `Scala` is a plain caption and localizes.
2. Is it a number, a unit, or a readout value? EXEMPT per §5 (D-03).
3. **What ELEMENT receives it?** A readout node is never a `[data-i18n]`
   element — **exempt regardless of parameter type**. O-Marimba's six timbre
   words are `AudioParameterFloat`-backed, so arms 1 and 2 say localize, but
   they are written into the knob's readout node wearing a word instead of a
   number. Keying one makes the element enter and leave the sweep as the knob
   turns.

An arm-3 verdict can be OVERRULED with reasons — O-Gain's `LOW`/`MED`/`HIGH`
localizes because that node never holds a number, `learnConfidence` is not a
parameter, and a committed gate state drives all three French faces. State the
reasons in `I18N_EXEMPT` or in the commit message.

Every exclusion is an `I18N_EXEMPT` entry **with a reason** (§7, assertion 14).
A bare skip list lets a missed label hide as a deliberate one.

## Carried lessons that cost real time when ignored

- **`git commit -- <paths>` TAKES ONLY TRACKED FILES.** O-Marimba's first
  commit silently omitted three new untracked files and left HEAD with a
  gutted `index.html` and no controller. **`git add` the exact new paths,
  commit, then `git show --stat` and confirm the file count.** Every plugin in
  this stage adds new files.
- **Do not trust the plan's structural claims about your plugin.** Bridges,
  tip counts, text counts, positioner shape and served root were each wrong at
  least once in Stage J. Verify against the code. Report every divergence.
- **The title of a split node is the control's own existing English caption.**
  Reuse it, never author new prose. Verify every body byte-identical vs `HEAD`.
- **`flex: 1 1 0` WITH `min-width: 0`.** A width pin on one flex button just
  redistributes to the same row total — flex items are floored by min-content
  and carry `min-width: auto`. Verify any timing-sensitive measurement at two
  settle times (180ms and 1.7s) to rule out a transition mid-flight.
- **Try `width: 100%` on a wrapper first, and probe it moves zero children.**
  It frees natural French where the French is NARROWER; it does nothing for a
  shrink-wrapping box. Pin **per-element**, never uniformly, and pin to the
  **English** box rounded up so English barely moves.
- **Strings owned by shared registry modules under
  `${CMAKE_SOURCE_DIR}/modules/` are EXEMPT.** Localizing them is cross-plugin
  and a local edit is reverted by `/module-upgrade`. Name the module in the
  exempt reason.
- **A `screencapture` is NEVER the verification.** One Stage J run grabbed the
  user's entire desktop including an unrelated private document. Use the
  headless harness and DOM reads. Do not run `screencapture` for any reason.
- **`serve-ui.js` picks port 0 deliberately** — a server on a taken port
  silently serves another session's files. Never pin a port.
- **A top-level init touching a lower `let`/`const` is a TDZ throw that kills
  every later initializer.** Move the i18n block only where there is no
  `init()`-last discipline, and check the console for a ReferenceError.
- **`build-and-install.sh` builds VST3+AU only** — the Standalone `.app` stays
  STALE. If you test the Standalone, build `<Name>_Standalone` explicitly, and
  check the binary with `strings` rather than trusting the run.

## Geometry

Run the label gate's before/after diff. `align-items: center` is the tell for
a row that shifts. **Also scan for French getting SHORTER** — four of twelve
findings in one stage were shrink, not growth.

Every geometry pin must be **reverted alone and confirmed to re-break the
gate**. A pin whose negative control passes is DECORATION — remove it or
re-label it a design pin. Do not claim it as a fix.

For any "no failures" verdict from a sweep, run a **harness-blindness check**:
break something the sweep should catch and confirm it reports. Without that
control, "no failures" is indistinguishable from a sweep that cannot see them.

## Gates — all must pass before the commit

```bash
node scripts/check-i18n.js --plugin <Name> --strict-v2
node scripts/check-ui-labels.js --plugin <Name>
node scripts/boot-all-uis.js            # the only gate that sees a swallowed binding failure
ls plugins/<Name>/tests/ 2>/dev/null    # if present, run every gate in it
./scripts/build-and-install.sh <Name>
```

`boot-all-uis.js` must stay green for **every** plugin, not just yours.

## Commit discipline

Trunk-based, on `main`, in one shared checkout. Another session may be writing
inside your plugin.

```bash
git branch --show-current          # immediately before, not once at start
git status --short
git add <exact new paths>          # untracked files are NOT picked up by a pathspec commit
git commit -m "..." -- <exact file list, NOT the directory>
git show --stat HEAD               # confirm the file count and that nothing foreign rode along
```

**The scope that matters is the FILE SET, not the directory.** `git commit --
plugins/<Name>` is not narrow enough on a plugin another session is editing.

One commit per plugin. A repo-level gate fix lands as its **own commit ahead
of** the plugin that needed it — never folded into a plugin commit.

Do **not** tag. Do **not** push. `/publish` is the only thing that tags.

Commit message shape:
```
improve: <Name> vX.Y.Z - the PAGE speaks French (Stage K batch <N>, canon v2)
```
followed by what was found, what diverged from the plan, what was deliberately
not fixed and why, and the geometry result.

## What to report back

A compact report, not a transcript:
- version shipped + commit sha + `git show --stat` file count
- label / attribute / JS-string counts **as measured**, vs the plan's numbers
- every divergence from the plan's structural claims
- geometry: moved-before / moved-after, every pin with its negative control
- `I18N_EXEMPT` entries and the arm of the D-01 test each rests on
- anything found and deliberately NOT fixed, with the reason
- what is NOT verified

---

# PARALLEL DISPATCH PROTOCOL

Batches K1, K3 and K4 run several executors at once in the SAME checkout.
(K2 — the five tight frames — stays serial by plan instruction, because its
stop condition depends on seeing each geometry diff before the next plugin.)

Three resources are shared and would corrupt or lose work under concurrency.
Each has a rule.

## 1. `build/` — SERIALIZED BY A MUTEX

`build-and-install.sh` hard-codes ONE build directory for every plugin
(`scripts/build-and-install.sh:308,443`) and ninja does not lock. Two
concurrent invocations share `.ninja_deps` and `.ninja_log`.

Wrap **the build and everything that touches the AU cache** — which is also
global, `killall -9 AudioComponentRegistrar` plus a wipe of
`~/Library/Caches/AudioUnitCache` — in this mutex:

```bash
LOCK=/tmp/claude-501/stagek-build.lock
# wait for the lock, breaking it if a dead executor left it held >25 min
while ! mkdir "$LOCK" 2>/dev/null; do
    if [ -n "$(find "$LOCK" -maxdepth 0 -mmin +25 2>/dev/null)" ]; then
        echo "breaking a stale build lock"; rmdir "$LOCK" 2>/dev/null
    fi
    sleep 15
done

./scripts/build-and-install.sh <Name>        # and auval, and any DAW-facing check

rmdir "$LOCK"                                 # RELEASE IMMEDIATELY. Do not hold
                                              # it across authoring or gates.
```

**Release the lock the moment the build and `auval` are done.** If you abort
for any reason, release it first. Everything else — reading, authoring French,
`check-i18n`, `check-ui-labels`, `boot-all-uis` — runs OUTSIDE the lock.
`serve-ui.js` picks port 0, so the browser gates are already concurrency-safe.

## 2. `PLUGINS.md` — THE ORCHESTRATOR OWNS IT

**Do not edit `PLUGINS.md`.** Every plugin commit used to carry its own row,
but two concurrent read-modify-writes lose one of them silently, and this file
is exactly one row per plugin with no merge help inside a single working tree.

Report your new version number and the row content you would have written. The
orchestrator updates every row for the batch in one commit after the batch
lands, then runs the duplicate check.

## 3. `scripts/` — THE ORCHESTRATOR OWNS IT

**Do not edit anything under `scripts/`.** Gate defects are frequent in this
task — four in Stage J, three already in Stage K — so two executors editing
`check-i18n.js` at once is a live hazard rather than a theoretical one.

When you find a gate defect: **stop, and report it** with the shape of the
wrong assumption, the negative control that proves it, and whether your plugin
is blocked by it. The orchestrator lands the fix as its own commit ahead of
your plugin, per the standing precedent, and tells you to resume.

If the defect BLOCKS you, say so plainly and stop rather than working around
it. A workaround in one plugin is the thing that hides a repo-wide gate hole.

## 4. THE SCRATCHPAD IS SHARED — NAMESPACE EVERY FILE YOU WRITE

The scratchpad path carries ONE session id for every executor in the batch, so
a bare-named file in its root is not yours. This is not hypothetical: in the K1
batch, O-Freeze's `measure.js` was overwritten mid-run by O-Bassoon's script and
handed O-Freeze a page of O-BASSOON's knob widths in reply to its own command,
and its baseline JSON was clobbered in the same way.

**Write everything into `scratchpad/<yourplugin>/`.** Never write a bare
`before-en.json`, `after-en.json`, `geom.js`, `measure.js` or `probe.js` at the
scratchpad root — every executor reaches for those same names at the same time.

The danger is not a crash, it is a **silent wrong number**. A geometry diff
taken between a baseline that was clobbered and an after-pass that was not is a
comparison between two different plugins, and it reads exactly like a real
result. Geometry numbers are the primary evidence of this stage, so a wrong one
is worse than a missing one. If you suspect a file was touched, re-run the
measurement rather than reasoning about whether it mattered.

## Commits under concurrency

`git commit -- <exact paths>` is safe while another executor stages files: a
pathspec commit takes only the paths you name, whatever else sits in the index.
Keep naming the exact file set, never the directory.

The one transient failure you may see is `Unable to create '.git/index.lock'`
— another executor's git call, holding it for a moment. **Wait a few seconds
and retry.** Do not delete the lock file; it is not stale, it is in use.
