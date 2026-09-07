# Quick Task 260906-s71 — Task 1 Summary

**Task:** Teach check-i18n to see CMake-embedded module JS, then localize the shared
scala-tuning-engine panel and all five consumer tables in ONE change set.

**Status:** COMPLETE. One path-scoped commit, `842bf6e0`, on `main`.
**Executed:** 2026-09-06

---

## 1. Precondition — FIRED, not assumed

The plan required all five to pass `check-i18n` at exit 0 and report `1 module(s)`
on the tree as found. Fired (wave 4f instruction 3):

| Plugin | exit | modules scanned | LABELS as found |
|---|---|---|---|
| O-Bassoon | 0 | `1 module(s)`: inline `<script type="module">` | 36 |
| O-Bowed | 0 | `1 module(s)` | 45 |
| O-Contrabass | 0 | `1 module(s)` | 57 |
| O-Reed | 0 | `1 module(s)` | 59 |
| O-Wind | 0 | `1 module(s)` | 71 |

Matches the plan's baseline table exactly. **Precondition MET.**

---

## 2. Step A — the gate extension

`scripts/check-i18n.js` now adds to `pageModules` every
`${CMAKE_SOURCE_DIR}/modules/**.js` the plugin's `CMakeLists.txt` embeds, resolved
against `repoRoot`. Derived by scanning the build file, never a transcribed
filename list — the same rule the file already argues for its directory scan.

- CMake `#`-comment lines are dropped first, so a commented-out reference is not
  read as embedded.
- Deduped by resolved path **and** by code, so a file also reachable through
  `js/` is not scanned twice.
- `.bundle.js` / `.min.js` route through the existing `bundlesSkipped` path.
- A path CMake names that is **not on disk** raises a `[12]` check failure rather
  than silently shrinking the scanned set.

**Census re-derived from the tree before acting** (`grep -rn 'CMAKE_SOURCE_DIR}/modules' plugins/*/CMakeLists.txt | grep '\.js'`):
10 references, 8 plugins, 5 distinct files — identical to the plan's table.

---

## 3. Step B — what the extended gate exposed repo-wide (the positive control)

Fired **unrestricted, repo-wide, before a byte of the module was keyed.**
Exit **7**. The extension is **NOT inert**:

| Plugin | `[12]` findings | newly-scanned source |
|---|---|---|
| **O-Bassoon** | **39** | tuning-panel.js |
| **O-Bowed** | **39** | tuning-panel.js |
| **O-Contrabass** | **46** | tuning-panel.js **+ preset-manager.js** |
| O-Marimba | 5 | analog-eq-unit.js, compressor-unit.js |
| **O-Reed** | **39** | tuning-panel.js |
| O-ReverseDelay | 6 | preset-manager.js |
| **O-Wind** | **39** | tuning-panel.js |

### The five FAIL lines, verbatim (pre-localization RED)

```
FAIL: [O-Bassoon (Resources/ui)] [12] no prose string is written to textContent / innerText outside setLabel — 39: modules/tuning/scala-tuning-engine/js/tuning-panel.js:88 "Intervals (" | modules/tuning/scala-tuning-engine/js/tuning-panel.js:88 "notes)" | modules/tuning/scala-tuning-engine/js/tuning-panel.js:97 "Circle" | modules/tuning/scala-tuning-engine/js/tuning-panel.js:98 "Polar" | modules/tuning/scala-tuning-engine/js/tuning-panel.js:99 "Matrix"
FAIL: [O-Bowed (Resources/ui)] [12] no prose string is written to textContent / innerText outside setLabel — 39: modules/tuning/scala-tuning-engine/js/tuning-panel.js:88 "Intervals (" | ... :98 "Polar" | ... :99 "Matrix"
FAIL: [O-Contrabass (Source/ui/public)] [12] no prose string is written to textContent / innerText outside setLabel — 46: modules/persistence/preset-manager/js/preset-manager.js:422 "Default" | :424 "Load" | :425 "Save" | :421 "Previous preset" | :423 "Next preset"
FAIL: [O-Reed (Resources/ui)] [12] no prose string is written to textContent / innerText outside setLabel — 39: modules/tuning/scala-tuning-engine/js/tuning-panel.js:88 "Intervals (" | ... :98 "Polar" | ... :99 "Matrix"
FAIL: [O-Wind (Resources/ui)] [12] no prose string is written to textContent / innerText outside setLabel — 39: modules/tuning/scala-tuning-engine/js/tuning-panel.js:88 "Intervals (" | ... :98 "Polar" | ... :99 "Matrix"
```

Full capture: `scratchpad/s71-stepB-repowide-BEFORE.txt`.

### Disposition — the NARROWING arm was taken, and why

The out-of-scope findings are **rendered English with no key** — `Load`, `Save`,
`Previous preset`, `Next preset`, `Default`, `EQ`, `COMP`, `GR` — not a one-line
trivial fix touching no rendered string. They therefore fail the first arm's own
test. `preset-manager.js` alone has eighteen consumers, so keying it is a rollout;
shipping it inside a tuning-panel change would put four unrelated plugins' UIs in
one commit.

Implemented as the plan's pre-authorized fallback: a named, commented constant
`CMAKE_MODULE_JS_SCOPE = /(^|\/)tuning-panel\.js$/`, carrying the reason, the
seven-plugin red set, and a TODO naming the four still-unscanned module files with
their consuming plugins. It is a **basename regex, not a path allowlist**, so a new
consumer embedding the panel from a different directory is still picked up — the
"pass by not looking" shape the file argues against everywhere else.

### The control SURVIVED the narrowing

Re-fired repo-wide. Exit **5** — exactly the five consumers still RED at **39
findings each**, the three out-of-scope plugins back to green, and `[12]` now
reading `2 module(s): the inline <script type="module"> in index.html,
modules/tuning/scala-tuning-engine/js/tuning-panel.js` on all five.

A gate that cannot fail on the unlocalized module cannot certify the localized one.
It could, and it did.

**Still unscanned by 12/13/15 for their consumers** (handed to Task 3's deferred-items):
`preset-manager.js` (O-Contrabass, O-ReverseDelay), `analog-eq-unit.js` +
`compressor-unit.js` (O-Marimba), `webview-drop-streaming.js` (O-MicrotonalSampler —
already **clean** under the unrestricted follow; excluded only because the scope is
one regex).

---

## 4. Steps C/D — the module at v3.1.0

37 `data-i18n` keys, adopting MTS's key names verbatim. Attributes and the hook
only — `tuning-panel-root` and `tuning-center-column` wrappers untouched, since
O-Contrabass ships its own `.tuning-panel` overrides and O-Bowed mounts the panel
in an overlay.

- `refreshPanelI18n()` calls `window.__reapplyI18n` when it is a function, guarded
  for a missing hook, at **6** injection sites: `render()`, `updateIntervalList()`,
  `drawTrueKeys()`, `drawRotationTable()`, `renderLibraryList()`, `setGeneratorType()`.
- **3** `data-i18n-vars` sites: the static and dynamic `label.intervalsCount`, and
  `label.noteCount`.
- The static interval-list header's nested `<span id="interval-count">` is dropped
  (`applyLabel` writes `textContent` and would destroy it); the
  `<div class="interval-list" id="interval-list">` element itself is kept, and
  `updateIntervalList()`'s `countEl` write is already `if (countEl)`-guarded.
- Version: header `v3.0.1`→`v3.1.0`; `module.yaml` `2.1.0`→`3.1.0` with a changelog
  entry; `registry.yaml` `3.0.1`→`3.1.0`. The stale `used_by` was **left as found**
  by decision and flagged in a comment as not-to-be-driven-from.

---

## 5. Step E — 37 rows in five consumers

| Plugin | LABELS before | after | expected | languages |
|---|---|---|---|---|
| O-Bassoon | 36 | **73** | 73 | en, fr, zh-Hans |
| O-Bowed | 45 | **82** | 82 | en, fr |
| O-Contrabass | 57 | **94** | 94 | en, fr |
| O-Reed | 59 | **96** | 96 | en, fr |
| O-Wind | 71 | **108** | 108 | en, fr |

"Verbatim copy" was **proven, not asserted**: a Node script evaluated MTS's and each
consumer's `LABELS` and byte-compared all 37 `en`/`fr` strings, the `sameAsEn` flags,
and O-Bassoon's `zh-Hans` strings + `reviewed:'bt'`. Final result:
`VERBATIM: all 37 rows byte-identical to MTS in en/fr, +zh on O-Bassoon only`.

`i18n-zh-lint --plugin O-Bassoon --verbose`: **0 findings**, 86 zh entries checked,
0 at `'mt'`. **Zero Z5 collisions**, so the plan's M13 blind-reverse-read trigger
never fired — nothing to re-read.

---

## 6. Three defects found, none of them in the plan

### D1 — O-Contrabass `label.loadScl` collision (would have shipped a regression)

O-Contrabass **already owned** `label.loadScl` for its own `#scl-load-btn`
(`index.html:1250`, "Load .scl…" / "Charger .scl…"). The module's row, inserted
later in the same object literal, **silently overwrote it** — duplicate object key,
last one wins — changing a shipping button's French to "Ouvrir .SCL" and losing both
the ellipsis and the reviewed wording.

**Caught by the plan's own arithmetic:** 57 + 37 should be 94; the gate read **93**.
Nothing else would have flagged it — check-i18n passed at 93.

Fixed by renaming the *plugin-local* key to `label.loadSclFile` (one row + one
`data-i18n` attribute); the shared module vocabulary keeps `label.loadScl`. This
added `plugins/O-Contrabass/Source/ui/public/index.html` to Task 1's commit — a file
not in Task 1's `<files>` list. Recorded as a deviation.

### D2 — "copied verbatim" was FALSE on two strings

MTS's French counted captions use **U+00A0** before the colon; the first
transcription wrote **U+0020**. `i18n-fr-lint` **exit 2**, ten `T4` findings
(`label.intervalsCount` + `label.noteCount` × 5 plugins). Fixed, then proven
byte-identical. fr-lint now exit 0.

This is exactly the failure the "copy, not authoring" claim invites — asserting a
copy instead of measuring one. The byte-comparison in §5 exists because of it.

### D3 — three I18N_EXEMPT entries became false claims

O-Bassoon, O-Reed and O-Wind carried `I18N_EXEMPT` entries stating the tuning tab is
English in both languages and that localizing it is out of scope — O-Wind's read
*"O-WIND'S TUNING TAB IS THEREFORE ENGLISH IN BOTH LANGUAGES … a French user meets an
English page on roughly a third of this plugin's navigable surface."* This change
makes all three false. Replaced (not amended) with the D-02 exemption for
`12-TET Standard`, scoped `#scale-name-display`, mirroring MTS's precedent: it is the
tuning name `getTuningName()` reports, written into `.scl` files and exported HTML —
data, not copy. O-Bowed and O-Contrabass, which carried no such claim, gained only
the exemption.

The `12-TET Standard` literal was the single remaining `[12]` finding after keying
(39 → 1 on each of the five). Exempting rather than emptying the static shell follows
MTS: `updateScaleNameDisplay()` is called only from inside `loadInitialState()`'s
`try`, so an empty shell would blank the box on a backend failure.

---

## 7. Plan predictions that turned out FALSE

1. **"The paragraph under assertion 12/15 that currently explains why sibling modules
   are scanned"** — no such paragraph exists at the top of `check-i18n.js`; that
   explanation lives inline at ~line 800. Did the honest equivalent: rewrote assertion
   12's entry and added a new "WHAT 12, 13 AND 15 ACTUALLY SCAN" block.
2. **`modules/registry.yaml` has no per-module `changelog` key** — the plan called for
   "a changelog line". No entry in the file has one. Used a comment block above the
   version instead, and bumped the registry's own `version`/`last_updated` per the
   file's own mandatory NOTE.
3. **O-Contrabass LABELS after = 94** — arithmetically right, but only after fixing a
   collision the plan did not anticipate. As written the change produced **93**.
4. **Verify command 4c is not executable as spelled.**
   `awk '/name: scala-tuning-engine/,/^  - name: /'` collapses to a **single line**:
   awk tests the range's END pattern on the START line, and
   `  - name: scala-tuning-engine` matches `^  - name: `. The command can never see the
   version line and fails regardless of the file's contents. The registry *is* correct
   at 3.1.0. Re-ran with `awk '/- name: scala-tuning-engine/{f=1;next} f&&/^  - name: /{f=0} f'`;
   everything else in the block ran verbatim. (Repo pattern:
   `pattern_recorded_gate_command_not_executable_as_spelled`.)
5. **`git commit -F <msg> -- <paths>` ordering** — the plan's commit block shows the
   pathspec before the message; `-m`/`-F` must precede `--` or git reads the message as
   a pathspec. First attempt errored and committed nothing.
6. **O-MicrotonalSampler "may go red"** — it did **not**. `webview-drop-streaming.js`
   was newly scanned under the unrestricted follow and is clean.
7. **The plan's step-B table said O-Contrabass would show `preset-manager.js,
   tuning-panel.js`** — correct, but it did not predict that this makes O-Contrabass's
   red *un-clearable* by this task alone, which is part of why the narrowing arm was
   forced rather than merely preferred.

---

## 8. Gate results (all fired in the background; 600 s watchdog respected)

| Gate | Result |
|---|---|
| `check-i18n --plugin` × 5 | **exit 0**, each naming the module in `[12]` as `2 module(s)` |
| `check-i18n` repo-wide | **exit 0** — `ALL CHECKS PASS`, 43 localized plugins |
| `i18n-fr-lint` | **exit 0** — `CLEAN` |
| `i18n-zh-lint --plugin O-Bassoon` | **exit 0** — 0 findings, 0 Z5 |
| Plan verify block [1]–[8] | **TASK 1 VERIFY OK** (4c corrected as above) |

Untouchability, asserted with `git diff --quiet`:

- `plugins/O-MicrotonalSampler` — **byte-unchanged**
- `plugins/{O-Bowed,O-Reed,O-Wind}/tests/ui_tip_render_check.js` — **byte-unchanged**
  (wave 4f's N12 census not consumed)
- `modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` — **byte-unchanged**

---

## 9. Commit

**`842bf6e0`** — `feat(scala-tuning-engine): localize the shared tuning panel
en/fr/zh-Hans at module v3.1.0`

Path-scoped (`git commit -F <msg> -- <explicit paths>`), branch and
`git status --short` re-checked immediately before. 10 files, 612 insertions,
69 deletions. Another session's ` M .gsd/dispatch-isolation-sentinel.json` was
present throughout and was **not** swept in. No tag.

```
modules/registry.yaml                             |  19 ++-
modules/tuning/scala-tuning-engine/js/tuning-panel.js | 139 ++++++++++-----
modules/tuning/scala-tuning-engine/module.yaml    |  16 ++-
plugins/O-Bassoon/Resources/ui/js/i18n.js         |  80 +++++++++-
plugins/O-Bowed/Resources/ui/js/i18n.js           |  66 ++++++++
plugins/O-Contrabass/Source/ui/public/index.html  |   2 +-
plugins/O-Contrabass/Source/ui/public/js/i18n.js  |  74 ++++++++-
plugins/O-Reed/Resources/ui/js/i18n.js            |  75 ++++++++-
plugins/O-Wind/Resources/ui/js/i18n.js            |  84 +++++++++--
scripts/check-i18n.js                             | 126 ++++++++++++++-
```

---

## 10. Handed forward

**To Task 2:** the panel is keyed but still **lazily mounted** — until a tuning state
exists in each `tests/i18n-states.json`, all 37 captions remain invisible to
`check-ui-labels` and `measure-ui` in every language. Note `label.loadSclFile` is a
NEW key on O-Contrabass's own button; its geometry is unchanged (same caption text).

**To Task 3's deferred-items:**

1. **D1 CLOSED** by option 1. Wave 4f still owes the `zh-Hans` column on these 37 keys
   for O-Bowed, O-Reed and O-Wind — and for **O-Contrabass, which is not a 4f plugin**
   and will otherwise be missed. Plus the N12 enumeration-form census on the three
   untouched `ui_tip_render_check.js` files.
2. **The narrowing arm was taken.** Four module files remain unscanned by 12/13/15:
   `preset-manager.js` (O-Contrabass, O-ReverseDelay), `analog-eq-unit.js` and
   `compressor-unit.js` (O-Marimba), `webview-drop-streaming.js`
   (O-MicrotonalSampler — already clean). Deleting `CMAKE_MODULE_JS_SCOPE` turns
   `O-ReverseDelay` and `O-Marimba` red immediately; the TODO in `check-i18n.js` says so.
3. **`registry.yaml` `used_by` for scala-tuning-engine is STALE** — names O-Bells,
   O-Formant, O-IntonationPad (forks with private copies), omits O-Reed, O-Wind,
   O-MicrotonalSampler. Left as found, now commented as not-to-be-driven-from; rebuild
   it from the CMake grep.
4. **O-MicrotonalSampler untouched by decision.** The module and the MTS copy now share
   key names, so a future re-sync is a diff, not a redesign. Two keys exist only in the
   MTS copy: `label.totalSpan`, `label.tuningPanelUnavailable`.
5. **Seven false plan predictions** (§7). Wave 4e recorded fifteen; the count is itself
   the finding.
