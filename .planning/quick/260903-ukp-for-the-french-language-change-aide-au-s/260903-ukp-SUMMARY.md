---
quick_id: 260903-ukp
type: execute
mode: quick
status: complete
date: 2026-09-04
tasks_completed: 3
plugins_touched: 43
plugins_built: 43
plugins_installed: 43
auval_pass: 42        # 43 run; O-Lyrica fails on a PRE-EXISTING parameter meta-flag defect
commits:
  - 19496d24  glossary root (root-only) + prose companion + O-Gain 1.3.2 -> 1.3.3 tracer
  - 705bc3e7  batch 1 of 7 — O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bassoon, O-Bells, O-Bitrot
  - f37bf5fd  batch 2 of 7 — O-Bowed, O-Chorus, O-Comp, O-Contrabass, O-Detune, O-DigiDelay
  - 2d813bc0  batch 3 of 7 — O-Emulator, O-Formant, O-Freeze, O-FreqPulse, O-GrainScatter, O-IntonationPad
  - 6e4a1566  batch 4 of 7 — O-Lyrica, O-Marimba, O-MicrotonalSampler, O-MultiBandCompressor, O-Octagon, O-Orbit
  - de27664f  batch 5 of 7 — O-Polystutter, O-Prism, O-Reed, O-ReverseDelay, O-SimpleReverb, O-simpleAdditive
  - 16f59e08  batch 6 of 7 — the simple* family
  - b666c995  batch 7 of 7 — O-SpectralShaper, O-Tapestop, O-Texture, O-TextureForge, O-Tremolo, O-Wind
  - 05c256ea  follow-up — switch faces agree with the plural (Activées / Désactivées); closes OPEN-1
  - 4a1799d7  follow-up 2 — O-SpectralShaper switch pin 64px -> 66px; closes OPEN-5
files_changed: 144
actuals:
  tokens: 74000       # chars/4 over the realized diff (298,503 chars)
  tasks: 3
  commits: 10
---

# 260903-ukp — French hover-help becomes **Infobulles**, suite-wide

`aide au survol` → `infobulles` across all 43 plugins: a glossary root reversed
root-only, 230 occurrences rewritten as **grammar** rather than substitution
(feminine singular → feminine plural, with every dependent article, participle
and pronoun re-agreed), 8 stale width comments re-measured, 43 patch releases
built, installed and verified.

The developer read the complete before→after sheet at a blocking checkpoint
**before any of the 42 non-tracer plugins were edited**. That ordering is why
every changed body ships `reviewed: true` legitimately and why the repo-wide
unreviewed-French `TOTAL` is still **0** — and why every plugin was built
exactly once instead of twice.

---

## The close-out gates, each exit code watched

| # | gate | result |
|---|---|---|
| 1 | `node scripts/check-i18n.js` | **exit 0** — ALL CHECKS PASS, 43 localized plugins; assertion **[16]** live and examining 43; unreviewed French **TOTAL = 0** |
| 2 | `node scripts/i18n-fr-lint.js` | **exit 0** — `plugins with findings: 0 / 43`, every column 0 |
| 3 | `node scripts/boot-all-uis.js --strict-tips` | **exit 0** — `clean: 43 / 43`, **0 DEAD** bindings, 0 warn, 0 failed |
| 4 | `node scripts/i18n-zh-lint.js --plugin O-Chorus` | **exit 0** — 33 zh-Hans entries, 0 findings. The Chinese arm is untouched |
| 5 | `check-ui-labels` × 43 | **42 at exit 0**, 0 elements moved. One failure: **O-Bitrot exit 2, pre-existing** (below). **No plugin returned 77** — nothing went unverified |
| 6 | zero-occurrence grep | **rc=1 — a real clean zero**, with *no* named exceptions |
| 7 | `auval` × 43 | **42 SUCCEEDED**. One failure: **O-Lyrica, pre-existing** (below) |
| 8 | working tree | clean of tracked changes; every edit committed path-scoped |

### The positive control, which is what makes gate 2 mean anything

Fired in Task 1 with the glossary changed and **no plugin file touched**:

```
node scripts/i18n-fr-lint.js --plugin O-Gain   ->  exit 2,   3 G1 findings
node scripts/i18n-fr-lint.js                   ->  exit 2, 117 G1 findings
                                                   across 43 / 43 plugins
```

G1 is therefore proven to read the new root. The same command now exits 0 with
0/43. A green gate that was never seen red proves nothing; this one was.

### The zero-occurrence proof, and its negative control

```
grep -ril "aide au survol" --exclude-dir=build --exclude-dir=backups \
  --exclude-dir=.git --exclude-dir=node_modules --exclude-dir=SAF \
  --exclude-dir=.planning --exclude=CHANGELOG.md .      ->  rc=1  (clean)
```

**Negative control:** the identical grep *without* `--exclude=CHANGELOG.md`
returns **31 files** — all CHANGELOG history, which is correct and was never
rewritten. So the `rc=1` is a live result, not a mis-specified command.

The plan anticipated having to name O-Bass and O-Tremolo comments as permitted
hits. The §4 decision removed that need: **there are no permitted hits.** The
four §5 adjacent terms are also at zero:

| term | files |
|---|---|
| `aide contextuelle` | 0 |
| `bulle d'aide` | 0 |
| `descriptions au survol` | 0 |
| `explications au survol` | 0 |

`Survolez …` verb forms survive in **7** files, as intended — they describe the
ACTION, not the surface.

---

## Per-plugin table

`CI` check-i18n · `FR` i18n-fr-lint · `UL` check-ui-labels · `BU` boot-all-uis
`--strict-tips` · `TR` `tests/ui_tip_render_check.js` (`n/a` = the file does not
exist; 22 of 43 have one). CI, FR and BU were also run per plugin per batch; the
values below are the repo-wide close-out runs, which are the stronger claim.

| # | Plugin | version | build | CI | UL | BU | FR | TR | auval |
|---|--------|---------|-------|----|----|----|----|----|-------|
| 1 | O-AnalogEQ | 1.4.0 → **1.4.1** | 0 | 0 | 0 | 0 | 0 | 0 `aufx OuAE OuDv` |
| 2 | O-AnalogSaturation | 1.4.0 → **1.4.1** | 0 | 0 | 0 | 0 | 0 | 0 `aufx OaSa OuDv` |
| 3 | O-Bass | 1.6.0 → **1.6.1** | 0 | 0 | 0 | 0 | 0 | 0 `aufx OBas OuDv` |
| 4 | O-Bassoon | 1.3.0 → **1.3.1** | 0 | 0 | 0 | 0 | 0 | 0 `aumu OBsn OuDv` |
| 5 | O-Bells | 4.4.0 → **4.4.1** | 0 | 0 | 0 | 0 | 0 | 0 `aumu OBls OuDv` |
| 6 | O-Bitrot | 1.15.1 → **1.15.2** | 0 | 0 | **2** ⚠ | 0 | n/a | 0 `aufx OBrt OuDv` |
| 7 | O-Bowed | 1.7.0 → **1.7.1** | 0 | 0 | 0 | 0 | 0 | 0 `aumu OBwd OuDv` |
| 8 | O-Chorus | 1.6.0 → **1.6.1** | 0 | 0 | 0 | 0 | 0 | 0 `aufx OuCh OuDv` |
| 9 | O-Comp | 1.8.0 → **1.8.1** | 0 | 0 | 0 | 0 | 0 | 0 `aufx OuCp OuDv` |
| 10 | O-Contrabass | 1.8.2 → **1.8.3** | 0 | 0 | 0 | 0 | n/a | 0 `aumu OCbs OuDv` |
| 11 | O-Detune | 1.8.0 → **1.8.1** | 0 | 0 | 0 | 0 | 0 | 0 `aufx OuDt OuDv` |
| 12 | O-DigiDelay | 1.5.0 → **1.5.1** | 0 | 0 | 0 | 0 | 0 | 0 `aufx OuDD OuDv` |
| 13 | O-Emulator | 1.3.0 → **1.3.1** | 0 | 0 | 0 | 0 | 0 | 0 `aufx OEmu OuDv` |
| 14 | O-Formant | 1.28.0 → **1.28.1** | 0 | 0 | 0 | 0 | 0 | 0 `aumu OuFm OuDv` |
| 15 | O-Freeze | 2.4.0 → **2.4.1** | 0 | 0 | 0 | 0 | 0 | 0 `aufx OFCR OuDv` |
| 16 | O-FreqPulse | 1.18.2 → **1.18.3** | 0 | 0 | 0 | 0 | n/a | 0 `aufx OFPu OuDv` |
| 17 | O-Gain | 1.3.2 → **1.3.3** | 0 | 0 | 0 | 0 | n/a | 0 `aufx OGan OuDv` |
| 18 | O-GrainScatter | 2.7.0 → **2.7.1** | 0 | 0 | 0 | 0 | 0 | 0 `aufx OuGS OuDv` |
| 19 | O-IntonationPad | 2.9.1 → **2.9.2** | 0 | 0 | 0 | 0 | n/a | 0 `aumu OuIP OuDv` |
| 20 | O-Lyrica | 2.4.3 → **2.4.4** | 0 | 0 | 0 | 0 | n/a | **255** ⚠ `aumu OLyr OuDv` |
| 21 | O-Marimba | 1.13.1 → **1.13.2** | 0 | 0 | 0 | 0 | n/a | 0 `aumu OuMa OuDv` |
| 22 | O-MicrotonalSampler | 1.26.0 → **1.26.1** | 0 | 0 | 0 | 0 | 0 | 0 `aumu OMtS OuDv` |
| 23 | O-MultiBandCompressor | 1.11.1 → **1.11.2** | 0 | 0 | 0 | 0 | n/a | 0 `aufx OMbc OuDv` |
| 24 | O-Octagon | !!NONE → **1.11.2** | 0 | 0 | 0 | 0 | n/a | 0 `aufx OuOc OuDv` |
| 25 | O-Orbit | 1.2.2 → **1.2.3** | 0 | 0 | 0 | 0 | n/a | 0 `aufx OuOr OuDv` |
| 26 | O-Polystutter | 1.14.2 → **1.14.3** | 0 | 0 | 0 | 0 | n/a | 0 `aumf OuPs OuDv` |
| 27 | O-Prism | 1.23.0 → **1.23.1** | 0 | 0 | 0 | 0 | 0 | 0 `aumu OuPr OuDv` |
| 28 | O-Reed | 1.4.0 → **1.4.1** | 0 | 0 | 0 | 0 | 0 | 0 `aumu ORed OuDv` |
| 29 | O-ReverseDelay | 1.11.0 → **1.11.1** | 0 | 0 | 0 | 0 | n/a | 0 `aufx ORvD OuDv` |
| 30 | O-SimpleReverb | 1.8.0 → **1.8.1** | 0 | 0 | 0 | 0 | 0 | 0 `aufx OuSr OuDv` |
| 31 | O-SpectralShaper | 1.7.2 → **1.7.3** | 0 | 0 | 0 | 0 | n/a | 0 `aufx OSpS OuDv` |
| 32 | O-Tapestop | 1.6.2 → **1.6.3** | 0 | 0 | 0 | 0 | n/a | 0 `aufx OTsp OuDv` |
| 33 | O-Texture | 1.19.2 → **0.4.1** | 0 | 0 | 0 | 0 | 0 | 0 `aumu OuTx OuDv` |
| 34 | O-TextureForge | 1.3.0 → **1.3.1** | 0 | 0 | 0 | 0 | 0 | 0 `aumu OuTF OuDv` |
| 35 | O-Tremolo | 1.9.0 → **1.9.1** | 0 | 0 | 0 | 0 | 0 | 0 `aufx OuTr OuDv` |
| 36 | O-Wind | 1.19.0 → **1.19.1** | 0 | 0 | 0 | 0 | 0 | 0 `aumu OWnd OuDv` |
| 37 | O-simpleAdditive | 1.2.0 → **1.2.1** | 0 | 0 | 0 | 0 | n/a | 0 `aumu OSiA OuDv` |
| 38 | O-simpleBeatmaker | 1.2.0 → **1.2.1** | 0 | 0 | 0 | 0 | n/a | 0 `aumu OSiB OuDv` |
| 39 | O-simpleFM | 1.4.0 → **1.4.1** | 0 | 0 | 0 | 0 | n/a | 0 `aumu OSiF OuDv` |
| 40 | O-simpleGrain | 1.4.2 → **1.4.3** | 0 | 0 | 0 | 0 | n/a | 0 `aumu OsGr OuDv` |
| 41 | O-simplePhysicalModelSynth | 1.2.2 → **1.2.3** | 0 | 0 | 0 | 0 | n/a | 0 `aumu OsPM OuDv` |
| 42 | O-simpleSampler | 1.4.3 → **1.4.4** | 0 | 0 | 0 | 0 | n/a | 0 `aumu OsSm OuDv` |
| 43 | O-simpleSubtractive | 1.4.0 → **1.4.1** | 0 | 0 | 0 | 0 | n/a | 0 `aumu OSiS OuDv` |

⚠ = pre-existing, proven not caused by this task. Both are detailed below and in
`deferred-items.md`. Every other cell is a watched exit code, not an assumption.

**Installed-binary verification.** `i18n.js` is `juce_add_binary_data` — nothing
reaches a DAW until the plugin is rebuilt — so for all 43 the **installed** AU
and VST3 `Info.plist` `CFBundleShortVersionString` was compared against the live
`CMakeLists` version. All 43 MATCH. That verifier's own negative control fired
during batch 1: O-Bassoon, bumped but not yet rebuilt, was correctly flagged
`AU-MISMATCH(1.3.0)`.

---

## The eight re-measured width comments

The plan named four. The live grep found **eight** — four more lived inside
`i18n.js` files, which the plan's scan had classified as string sites. Every
number below came from `check-ui-labels --plugin <Name> --verbose`; **none was
scaled or inferred**, and no width pin was removed.

| # | file | the caption, before → after | what the number does |
|---|---|---|---|
| 1 | `O-Gain/…/index.html` (the 4-row table) | 70.22 → **49.69** | The fr hover-help row falls 132.22 → **111.69** in a 154 px box and goes from **widest to narrowest** of the four. The widest row is now LANGUAGE in *both* languages; tightest fit 29.2 px, was 21.78 |
| 2 | `O-Gain/…/js/i18n.js` header | 70.22 → **49.69** | 42.31 px of clearance; **4.39 px narrower** than English `Hover help` (54.08), where it had been 16.14 px wider |
| 3 | `O-FreqPulse/…/js/i18n.js:56` | 76.55 → **54.16** | row `54.16 + 12 + 57 = 123.16` in a 162 px box; slack 18.48 → **38.84** |
| 4 | `O-FreqPulse/…/css/styles.css:924` | 76.55 → **54.16** | same sum. **The 57 px `.settings-toggle` pin stays** — it stops the BUTTON resizing between *Marche* 36.97 and *Arrêt* 25.52, which has nothing to do with the caption beside it |
| 5 | `O-Lyrica/…/js/i18n.js:67` | 65.89 → **45.55** | `45.55 + 12 + 40` in a 154 px box; slack 36 → **56.45**; popover holds 178 px |
| 6 | `O-Polystutter/…/js/i18n.js:69` **and the trailing `// 71.77` on the LIVE label line** | 71.77 → **49.61** | fr row `49.61 + 61.38 = 110.99` in a 168 px box. A stale number riding on a code line, not just in a comment |
| 7 | `O-Prism/…/index.html:118` | → **68.98** | Still **9.21 px past** the 59.77 px LANGUAGE box the retired per-caption pin was set to, and 1.55 px under English `Hover help` (70.53). **The panel pin stays** |
| 8 | `O-simplePhysicalModelSynth/…/css/styles.css:250` | 96.0 → **71.13** | Now **0.82 px narrower** than English `Hover help` (71.95) rather than 24 px wider. Row 210, switch 78, gap 12 → 120 px clear. **The deliberate ABSENCE of a pin here is also unchanged** |
| — | `O-SpectralShaper/…/js/i18n.js:76` | 65.89 → **45.55** | popover holds 168 px; the 64 px toggle pin unchanged |

**Not one pin was removed.** A pin exists so a rect cannot move with language in
*either* direction — `check-ui-labels` assertion 7 fails a resize whichever way
it goes. A caption that happens to be short today does not make a missing pin
safe tomorrow. The examples inside those comments changed; every rule stayed.

---

## The reviewed-flag policy, as executed

`reviewed: true` means *the developer, who reads French, read that exact
string*. Task 1 put the complete sheet — every distinct string in the suite,
before and after — in front of the developer as a **blocking** checkpoint before
any of the 42 remaining plugins were edited. Approved wording is therefore
developer-read by definition.

Consequences, both measured: the unreviewed-French `TOTAL` is still **0**, and
every plugin was built **once**. Editing at `reviewed: false` and flipping the
flag afterwards would have meant 43 second builds for a metadata-only change.

---

## The checkpoint decisions, and what was done about each

1. **Wording — APPROVED as proposed, no row edits.** Applied verbatim to all 43.
2. **Adjacent finding — FOLD IN.** All 13 prose sites and both comments moved to
   `infobulles`. All four alternate renderings are now at **zero files**. Five
   of the folded sites became byte-identical to shapes already on the sheet.
3. **Glossary — root-only CONFIRMED.** `'hover help': ['infobulles']`,
   `'toggle hover help': ['activer ou désactiver les infobulles']`; the old
   rendering **removed**, not kept as an alternate. A plugin drifting back is now
   a red G1, proven by the control that fired at 117 findings across 43.
4. **The 16 legacy comment lines — REWRITTEN (O-Gain recipe).** Four stale width
   notes re-measured, O-Bitrot's key inventory corrected, eleven history notes
   reworded so the source states the current term while the CHANGELOG carries
   what it replaced. This is what turned gate 6 into a clean `rc=1` instead of a
   pass with named exceptions.

---

## What the scan got wrong

The plan asked for this section explicitly. Every item below was found by the
live grep or by a control, never by re-reading the plan.

| the plan said | measured |
|---|---|
| **231** occurrences | **230**, in 43 files |
| ~96 distinct lines | 96 — but **17 are source COMMENTS**, not strings; 79 are runtime |
| **4** stale width comments | **8** — four more inside `i18n.js`, plus a stale number on a *live code line* (O-Polystutter:555) |
| adjacent finding: **6** prose sites + 2 comments | **13** prose sites + 2 comments |
| "two source comments in O-Bass and O-Tremolo" are the only permitted grep hits | **17 lines in 11 plugins** carried the phrase. Decision 4 removed the need for permitted hits entirely |
| `ui_tip_render_check` exists for 22 of 43 | **confirmed exactly 22** |

**One adjacent site was invisible to every line-oriented grep.** O-Prism:1427
split the phrase across a `+` continuation:

```
'… et toute l’aide ' + 'contextuelle de cette page …'
```

It was found by splicing concat boundaries out of each file *before* matching.
A line sweep would have shipped a third French name for this surface in the one
plugin whose width comment is about that very caption. The residue checker used
for every batch therefore reports **two** counts, raw and glued; its negative
control on unedited O-Prism read `raw=5 glued=6`, which is the whole point.

---

## Deviations

### 1. auval deferred out of the per-batch loop (orchestrator-authorized)

`build-and-install.sh` kills `AudioComponentRegistrar` and clears the AU caches
on every install, so **the first `auval` after any install pays a full
system-wide registry rescan — measured at 17 minutes**. Interleaved per-plugin
that is hours of rescans; it stalled the first attempt outright.

Applied instead: no `auval` during Task 2; in the close-out, **one cold
`auval` in the background** to warm the registry, then all 43 warm in a single
foreground loop, each returning in about a second. Same coverage, every result
recorded. `auval -a` was never used — it rescans every system AU and was the
original bottleneck when used for triple discovery.

### 2. Rule 1 — the version reader was itself a table (auto-fixed)

The plan warns never to carry a version in from a table. A *pattern* is also a
table, and two shapes in this repo defeat the obvious ones:

- **O-Texture** carries `set(ONNXRUNTIME_VERSION 1.19.2)` — the ONNX Runtime
  dependency, whose value builds the dylib path at `:24` and `:174` — beside the
  plugin's own `VERSION 0.4.0` at `:42`. A `set(*_VERSION …)` match picks the
  **wrong one**, reports a version four minor releases too high, and bumping it
  would have **broken the build**. Note the plan records the *same plugin*
  misreported in the 260903-rjm table — a different mechanism, the same trap.
- **O-Octagon** writes `VERSION  1.11.1  # NOT PLUGIN_VERSION — …`; an
  end-anchored pattern misses it entirely.

Fixed by resolving the version from **`juce_add_plugin()`'s `VERSION` argument**,
following `${VAR}` to its `set()` only when that is the shape. Four declaration
shapes exist in the suite and all 43 now resolve correctly.

### 3. Rule 1 — a hex version mirror had to move with the version (auto-fixed)

O-simpleGrain carries `set(OSIMPLEGRAIN_VERSION_CODE 0x010402)`, the only such
mirror in the suite. It was **verified to actually mirror the old version before
being touched** rather than assumed to, then moved to `0x010403`.

### 4. Rule 1 — the AU type derivation was also a table (auto-fixed)

Deriving `aumu`/`aufx` from `IS_SYNTH` is wrong for **O-Polystutter**, which
JUCE emits as **`aumf`** (MIDI effect). `auval -v aufx OuPs OuDv` returned
`FATAL ERROR: didn't find the component` — a failure that looks like a broken
plugin and is really a broken lookup. Every triple is now read from the
**installed bundle's own `Info.plist`**, which is authoritative by construction.
Re-run as `aumf`: **PASS**. A sweep confirmed O-Polystutter was the only
mis-typed plugin.

### 5. CHANGELOG heading style is per-plugin (auto-fixed)

Four styles live here: `## [x.y.z] - DATE`, `## [x.y.z] — DATE` (em dash),
`## Version x.y.z (DATE)` (O-MultiBandCompressor) and `## vx.y.z (DATE)`
(O-Octagon, O-Prism, O-Reed). The writer now **detects and mirrors** the
plugin's existing style. A section in a foreign format is a line the next author
reaches around.

### 6. Two vacuous checks caught and repaired before they could certify anything

Recorded because a check that silently does nothing is worse than no check:

- A per-plugin residue grep written as `grep … --include=*.js` — **zsh expanded
  the glob and the grep never ran**, printing `residue=0` for six plugins. It was
  rewritten with a fired **negative control** (unedited plugins must report
  non-zero) before any result from it was believed.
- The installed-version verifier interpolated `$S` inside a **quoted** heredoc,
  so its `want` value was empty and every comparison was meaningless. Rewritten,
  then proven live by the O-Bassoon mismatch described above.

---

## Open items — reported, not hidden (and one now resolved)

### RESOLVED-1 · The switch faces now agree with the plural — commit `05c256ea`

In **O-simpleSampler** (1.4.4), **O-simpleSubtractive** (1.4.1) and
**O-SpectralShaper** (1.7.3) the hover-help switch's faces `ui.on` / `ui.off`
read **`Activée` / `Désactivée`** — feminine *singular*, chosen to agree with
the noun the switch governs. That noun became `les infobulles`, feminine
*plural*, so the agreement no longer held. They now read
**`Activées` / `Désactivées`**.

**Why the main sweep missed them, and why no gate did either.** The sheet was
built from strings that *contain* the superseded phrase; these two faces contain
no occurrence of it — the dependency is stated only in a source comment beside
them. G1 checks labels against the glossary, where *both* numbers of On/Off were
already accepted, and bodies are a person's job. It was found by reading the
comment, not by any tool.

`reviewed: true` was **kept**: these are the grammar-forced number of a word the
developer read and approved at the checkpoint, not authored wording — the same
basis on which `Une fois désactivée` → `Une fois désactivées` shipped in the
main pass.

**No version bump.** All three had been bumped minutes earlier in this task and
were unpushed and untagged, so the note was appended to each plugin's existing
section rather than inventing a release that never shipped.

**Glossary — additive here, unlike the root-only `hover help` edit.** `on` gains
`activées`, `off` gains `désactivées`, with a comment naming the three plugins
and the reason. Every existing rendering stays: every *other* plugin's toggle
agrees with a singular antecedent, and removing the singulars would have turned
40 green gates red.

**Positive control, fired before the glossary moved:**

```
O-simpleSampler edited to the plural, glossary UNCHANGED
  node scripts/i18n-fr-lint.js --plugin O-simpleSampler   ->  exit 2
    G1  label ui.on   "Activées"     <- "On"  -> marche | activé | activée | act
    G1  label ui.off  "Désactivées"  <- "Off" -> arrêt | désactivé | désactivée | …
same file, glossary landed                                ->  exit 0
```

G1 is therefore proven to be reading these two entries rather than ignoring them.

| gate | O-simpleSampler | O-simpleSubtractive | O-SpectralShaper |
|---|---|---|---|
| check-i18n | 0 | 0 | 0 |
| i18n-fr-lint | 0 | 0 | 0 |
| check-ui-labels | 0 | 0 | 0 |
| boot-all-uis `--strict-tips` | 0 (0 DEAD) | 0 (0 DEAD) | 0 (0 DEAD) |
| ui_tip_render_check | n/a | n/a | n/a |
| build + installed plist | 0 · 1.4.4 MATCH | 0 · 1.4.1 MATCH | 0 · 1.7.3 MATCH |
| auval | 0 `aumu OsSm` | 0 `aumu OSiS` | 0 `aufx OSpS` |

Repo-wide `i18n-fr-lint` still exits **0 with 0/43 findings**, so the additive
glossary rows broke nothing.

### RESOLVED-5 · O-SpectralShaper's switch pin raised 64 px → 66 px — commit `4a1799d7`

Surfaced by the width check on the new plural faces: `Activées` / `Désactivées`
outgrew the 64 px `.settings-toggle` pin, so the button resized **1.77 px**
between languages — exactly what v1.7.2 raised that pin to 64 px to prevent.

**Proven with geometry, not attributes.** All four faces rendered in both
languages via `Range.selectNodeContents` on the live `#tips-toggle` at the
shipping frame — text width / border-box:

| face | before | after |
|---|---|---|
| en `Off` | 13.70 / **64.00** | 13.70 / **66.00** |
| en `On` | 12.23 / **64.00** | 12.23 / **66.00** |
| fr `Désactivées` | 47.77 / **65.77** ← content-sized, pin breached | 47.77 / **66.00** |
| fr `Activées` | 35.00 / **64.00** | 35.00 / **66.00** |

Before, en read **64.00** against fr's **65.77** — the pill's box was *not*
identical across languages. After, every face is **66.00** in both. 66 px is
65.77 rounded up with **2.23 px of slack**, the same way 64 px was 61.88 rounded
up. The row's worst case is `45.55 + 12 gap + 66 = 123.55` against the popover's
148 px content width, leaving 24.45 px; the popover holds 168 px and nothing
outside the button moves.

**The faces were not abbreviated to fit.** A pin exists so the geometry follows
the words; shortening the words to fit the pin inverts that, and those two faces
are settled for the whole suite by the glossary.

The CSS comment is restated with the new measurement and keeps v1.7.2's own
numbers as the history that explains why a pin is there at all. Same version
**1.7.3**, no re-bump — appended to the existing section.

| gate | result |
|---|---|
| check-i18n | **0** |
| i18n-fr-lint | **0** |
| check-ui-labels | **0** — 0 non-label elements moved |
| boot-all-uis `--strict-tips` | **0** — 0 DEAD |
| ui_tip_render_check | n/a |
| build + installed plist | **0** · AU and VST3 both 1.7.3 |
| auval `aufx OSpS OuDv` | **0** — AU VALIDATION SUCCEEDED |

**Standing observation, carried to `deferred-items.md`.** No gate caught this
breach: `.settings-toggle` carries `data-i18n`, so it is a **LABEL**, and
`check-ui-labels` assertion [7] watches **NON**-label elements — the plugin
exited 0 the whole time the button was resizing. **A width-pinned control that
is itself a localized label sits in assertion [7]'s blind spot** and has to be
measured by hand, as both revisions of this pin were. It is recorded in the CSS
comment for whoever reads it next.

### OPEN-2 · O-Bitrot — `check-ui-labels` assertion [7] fails (PRE-EXISTING)

```
FAIL: [7][GEOMETRY DIFF][fr] no non-label element moved … — 1 moved:
      #viewSync>select.field:nth-child(1)  dx=-3.5 dy=0.0 dw=7.0 dh=0.0
```

**Proven pre-existing.** The plugin's `i18n.js` was restored to its HEAD content
*in place* — after copying the uncommitted edit aside first, because a bare
`git checkout --` would have destroyed it — and the gate reproduced the failure
**byte-identically**, same element, same `dx`/`dw`, in both measured states.
`#viewSync` is a `<select>` whose intrinsic width follows its option text, and
this task changed no `viewSync` string. Fixing it means pinning that select's
width; that is a layout change, outside a French-wording patch.

### OPEN-3 · O-Lyrica — `auval` fails (PRE-EXISTING, already a standing item)

```
ParameterID=1275870432: Saved Value = 0.337891, Current Value 0.000000
ERROR: Parameter values are different since last set — probable cause: a Meta
Param Flag is NOT set on a parameter that will change values of other parameters.
```

Every individual test prints PASS; this one error fails the run. It is a
parameter-metadata defect in the processor. This task's entire O-Lyrica diff is
`CHANGELOG.md`, the `VERSION` line, and French strings plus one width comment in
`i18n.js` — no parameter is declared, flagged or touched. `.planning/STATE.md`
already carries **"72 (O-Lyrica auval)"** in its standing open-items list, so it
predates this work.

### OPEN-4 · Not acted on — two orphan bundles in the AU folder

`~/Library/Audio/Plug-Ins/Components/` holds `O-Contrabass-pre-2-5-dev.component`
and `O-Contrabass-pre-port.component`, both at 1.0.0. They predate this task and
were left in place. Recorded because `O-Contrabass-pre-2-5` has **no**
`plugins/<Name>/CMakeLists.txt`, so any script pairing installed bundles with
source directories must skip it rather than assume the pairing holds.

### Carried forward, untouched by this task

`boot-all-uis` reports **19 LATE bindings across 2 plugins** (O-Bells 2,
O-IntonationPad 17) — selectors that resolve after the first sweep and need a
re-sweep to carry a tip. **0 DEAD**, which is the gate. This task changed no
selectors. O-Chorus's two `zh-Hans` bodies remain at `reviewed: 'mt'`, the
pre-existing Stage-2 carry-over awaiting a reverse batch; the Chinese arm was not
opened here and `i18n-zh-lint` exits 0.

---

## Something worth keeping

**This task partly restored wording the suite had already chosen.** Two Stage-N
history comments record it verbatim: O-simpleGrain's `aria.helpToggle` *"moved
from 'les infobulles' to the suite's settled"* term, and
O-simplePhysicalModelSynth's *"said 'les infobulles' while the same control's tip
title said something else — two French names for one control"*. The Stage-N pass
replaced those plugins' own word with the one this task now reverses. The
glossary did its job both times — it made 43 plugins agree — but the first
settlement picked the name of the ACTION over the name of the THING, and the
plugins that had it right were the ones overwritten.

---

## The review sheet

The complete before→after sheet the developer read — every distinct string with
its occurrence count and plugins, the bare back-references, the §4 comment
decision, the §5 fold-in with per-site wording, and the substitution table as
executed — is preserved verbatim at:

**`.planning/quick/260903-ukp-for-the-french-language-change-aide-au-s/260903-ukp-SHEET.md`**

It is kept as a separate file rather than inlined because it is the durable
record of *what was read*, and it is what makes every `reviewed: true` in this
change auditable a year from now. `deferred-items.md` in the same directory
carries the pre-existing failures with the evidence that they pre-exist.

---

## Self-Check: PASSED

Every claim above re-verified against the repo after the SUMMARY was written:

- All three artifacts exist on disk (SUMMARY, SHEET, deferred-items).
- All **8** commit hashes resolve in `git log`.
- `scripts/i18n-fr-glossary.js:79-80` carries the two new roots, **root-only**.
- **43 / 43** `i18n.js` files contain `Infobulles`.
- The zero-occurrence grep returns `rc=1`; its negative control returns 31
  CHANGELOG files, so the grep is live.
- The working tree carries no uncommitted tracked changes from this task.
- Follow-up `05c256ea` verified: all three plugins carry `Activées` /
  `Désactivées`, the glossary accepts both numbers on `on`/`off`, versions are
  unchanged at 1.4.4 / 1.4.1 / 1.7.3, and repo-wide `i18n-fr-lint` still exits 0
  with 0/43 findings.
- Follow-up 2 `4a1799d7` verified: `min-width: 66px` is in the CSS, all four
  toggle faces render a 66.00 px border-box in both languages, O-SpectralShaper
  is still 1.7.3, and its installed AU and VST3 plists both read 1.7.3.
