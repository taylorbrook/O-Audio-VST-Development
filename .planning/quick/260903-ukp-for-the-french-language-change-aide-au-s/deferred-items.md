# 260903-ukp — deferred items (out of scope, NOT fixed by this task)

## O-Bitrot — check-ui-labels assertion [7] fails, PRE-EXISTING

`node scripts/check-ui-labels.js --plugin O-Bitrot` exits **2**:

```
FAIL: [7][GEOMETRY DIFF][fr] no non-label element moved between English and fr
      at a fixed frame — 1 moved:
      #viewSync>select.field:nth-child(1)  dx=-3.5 dy=0.0 dw=7.0 dh=0.0
```

**Proven pre-existing, not caused by this task.** The plugin's `i18n.js` was
restored to its HEAD content in place (after copying the uncommitted edit aside
first — a bare `git checkout --` would have destroyed it) and the gate was
re-run: it produced the **byte-identical** failure, same element, same
`dx=-3.5 dw=7.0`, in both measured states. The edited file was then restored.

`#viewSync` is a `<select>` whose intrinsic width follows its option text; this
task changed no `viewSync` string. Fixing it means pinning that select's width
the way `.settings-popover` is pinned elsewhere in the suite — a change to
O-Bitrot's layout rules, which is outside a French-wording patch.

Not fixed here. O-Bitrot's `UL` cell in the SUMMARY table reads `2 (pre-existing)`
rather than `0`, and the repo-wide `check-ui-labels` sweep in Task 3 will list it.

## O-Lyrica — auval FAILS, PRE-EXISTING and already a standing project item

`auval -v aumu OLyr OuDv` exits 255. Every individual test prints PASS; the
failure is one error in the parameter sweep:

```
ParameterID=1275870432, Scope=0, Element=0: Saved Value = 0.337891, Current Value 0.000000
ERROR: Parameter values are different since last set — probable cause: a Meta
Param Flag is NOT set on a parameter that will change values of other parameters.
```

**Not caused by this task.** That is a parameter-metadata defect in the
processor. This task's entire O-Lyrica diff is `CHANGELOG.md`, the `VERSION`
line in `CMakeLists.txt`, and French strings + one width comment in
`Resources/ui/js/i18n.js` — no parameter is declared, flagged or touched
anywhere in it. `.planning/STATE.md` already carries **"72 (O-Lyrica auval)"**
in its standing open-items list, so it predates this work.

Fixing it means finding the parameter whose setter changes other parameters and
giving it `kAudioUnitParameterFlag_IsGlobalMeta` / `IsElementMeta` — a processor
change, outside a French-wording patch. Reported, not hidden: O-Lyrica's `auval`
cell in the SUMMARY table reads `255 (pre-existing)`.

## Not acted on — two orphan O-Contrabass bundles in the AU folder

`~/Library/Audio/Plug-Ins/Components/` holds `O-Contrabass-pre-2-5-dev.component`
and `O-Contrabass-pre-port.component`, both at 1.0.0. They predate this task and
are outside its scope; left in place deliberately. Noted only because the first
sweep of installed bundles reads them, and `O-Contrabass-pre-2-5` has no
`plugins/<Name>/CMakeLists.txt`, so any script that pairs installed bundles with
source directories must skip it rather than assume the pairing.

## Standing observation — assertion [7] cannot see a width-pinned LABEL

`check-ui-labels` assertion **[7] GEOMETRY DIFF** watches **non-label** elements
for language-driven movement. A control that is *itself* a `[data-i18n]` label
is therefore invisible to it — including when that control is width-pinned and
its own pin stops holding.

**Observed live in this task** (O-SpectralShaper, now fixed in `4a1799d7`).
`.settings-toggle` carries `data-i18n`, and when the French faces went plural
the pill went content-sized:

```
en  "Off"          box 64.00      (min-width: 64px held)
fr  "Désactivées"  box 65.77      (content-sized — the pin did not hold)
```

The button resized 1.77 px between languages — the exact class of regression the
pin exists to prevent — and `check-ui-labels --plugin O-SpectralShaper` **exited
0** throughout. Nothing was clipped, so assertion [4] stayed quiet too; only
[7] would have been the right gate, and [7] does not look at labels.

It was found by measuring the pill by hand before and after the wording change,
not by any gate.

**Consequence for anyone auditing width pins:** a pin on an element that carries
`data-i18n` is unverified by the suite's geometry gate. Every such pin in the
repo has to be re-measured by hand whenever the text it holds changes. Both
revisions of this one (40→64 at v1.7.2, 64→66 at v1.7.3) were hand-measured for
that reason.

**Possible closure**, not attempted here because it is a gate change rather than
a plugin fix: extend [7] to also compare the border-box of any `[data-i18n]`
element that carries an explicit `min-width` or `width`, which would catch a
breached pin without flagging the normal, expected text-width change on ordinary
captions.
