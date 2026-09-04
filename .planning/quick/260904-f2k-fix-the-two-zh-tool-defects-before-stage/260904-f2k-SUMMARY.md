---
phase: quick-260904-f2k
plan: 01
subsystem: i18n-tooling
status: complete
tags: [i18n, zh-Hans, lint, back-translation, opencc, provenance, blinding]

requires:
  - scripts/i18n-zh-lint.js (rule Z3 and its SELF_TESTS harness)
  - scripts/i18n-zh-glossary.js (the accepted rendering of `pan`)
  - scripts/i18n-zh-backtranslate.js (emit/ingest round trip)
provides:
  - scripts/gen-zh-trad-only.js (committed, re-runnable Z3 set generator with an offline --verify)
  - a corrected 3198-character Traditional-only set
  - an --ingest that refuses rather than running inert
  - a blinded --emit whose batches are unlinkable without their manifest
affects:
  - zh-Hans Stage 3 (the hard-case wave across 42 plugins)

tech-stack:
  added: []
  patterns:
    - "A derived literal whose derivation is unreadable is a hand-written literal wearing a provenance comment"
    - "The generator is the auditor of its own output — `--verify` re-reads the committed literal offline"
    - "Per-batch salted ids: blinding must make ingest IMPOSSIBLE without the manifest, not silently wrong"

key-files:
  created:
    - scripts/gen-zh-trad-only.js
  modified:
    - scripts/i18n-zh-lint.js
    - scripts/i18n-zh-backtranslate.js
    - research/i18n-zh-hans-localization.md

decisions:
  - "Reversed the original 'generator is a scratchpad throwaway' decision. That decision is the direct cause of the defect: an unreadable derivation could not be re-read, so nobody saw that it flagged the glossary's own rendering of `pan`."
  - "Excluded self ANYWHERE in the TS value list, not merely first. Excluding only on first-position would leave the 18 both-scripts characters (乾 徵 於 夥 …) as false positives of exactly the same class as 像."
  - "Salted the blinded ids PER BATCH rather than with a stable project key, so two emits of the same target share zero ids and a leaked batch cannot be cross-referenced against another batch."
  - "Kept the REAL id in the printed triples. The blinding exists to keep the source away from the reverse pass; the developer reading en/zh/en' side by side is the intended audience of that line."
  - "Generalized `refuse()` with an optional headline and remedy rather than adding a second refusal helper, so every new refusal keeps the tool's report-only exit-0 contract and its established voice."

metrics:
  duration: ~65min
  completed: 2026-09-04
  tasks: 3
  commits: 3

actuals:
  tokens: 17331    # chars/4 over the realized diff (69,323 chars) — the same
                   # computation as the plan's estimate.raw_tokens: 47500, NOT
                   # the estimate.tokens: 95000 figure. Came in ~2.7x under.
  tasks: 3
  commits: 3
---

# Quick Task 260904-f2k: Fix the two zh tool defects before Stage 3 — Summary

Three defects that Stage 3 would have hit on every one of 42 plugins are fixed, each behind a control that was observed failing first: the Z3 Traditional-only set no longer flags standard simplified characters (it lost 941 of them, including the 像 in the glossary's only accepted rendering of `pan`), `--ingest` now resolves its manifest deterministically and refuses loudly instead of announcing its own inertness and printing triples anyway, and `--emit` row ids are blinded behind a per-batch salt so the batch no longer hands the reverse pass the plugin name and the English key it claims to withhold.

## What was done

### Task 1 — the Z3 set generator and the corrected derivation (commit `6da6bd22`)

`scripts/gen-zh-trad-only.js` is new and **committed**. It both derives and audits the rule Z3 character set, in two modes:

- `--ts <path> --st <path> --target scripts/i18n-zh-lint.js` parses the two OpenCC dictionaries, applies the corrected rule, asserts the spotcheck in both directions, and rewrites only the region between the two `ZH_TRAD_ONLY_SENTINEL` markers. It refuses if either sentinel is missing or duplicated, if any TS key is not a single code point, or if the derived set misses the predicted 3198 characters / 9660 bytes.
- `--verify <path>` re-reads the committed literal back out by anchoring on the `new Set([...(` construct — not the surrounding comment, which legitimately contains quoted example characters — rebuilds the set and asserts the same spotcheck. No network, no dictionary files: this is the form a later reviewer can run.

**The rule was wrong, not the generator.** The old rule was `keys(TSCharacters) \ keys(STCharacters)`. The corrected rule adds a third clause: a TS key that appears **anywhere in its own value list** is excluded. `TSCharacters.txt` line 418 reads `像<TAB>像 象` — 像 survives T→S conversion unchanged, so it is a live simplified character no matter that the table has an entry for it. Such self-mapping keys never appear as ST keys, so the old two-clause rule could not see the class at all. 941 characters were in the set for exactly that reason.

Self is excluded anywhere in the value list rather than only in first position: 923 of the 941 map to themselves first, but the remaining 18 (乾 剋 劄 吒 夥 徵 扞 於 昇 氾 祕 脩 蒐 薹 袷 谿 釐 陞) map to themselves later and are the ambiguous both-scripts characters the provenance block already said must not be flagged.

Both dictionaries were re-fetched and their sha256 digests are byte-identical to the ones recorded on 2026-09-01 — `737c21c6…` and `a0ca1601…`. **The inputs did not drift; the rule was wrong.** Set: 4139 → 3198 characters, 12773 → 9660 bytes UTF-8, exactly as predicted.

The Z3 entry of `SELF_TESTS` now carries three violations (the original four-character string plus 說 and 龍 alone) and three controls (the original simplified string plus 声像 — the exact false positive — and 乾).

### Task 2 — deterministic manifest resolution (commit `9d5cff0b`)

The manifest path was derived in one expression from the file passed to `--ingest`, which is the **returned en' file**, while `--emit` writes its manifest beside its `--out` file. In the documented workflow (emit `/tmp/b.tsv`, ingest `/tmp/b.en.tsv`) those never coincide, so the lookup landed on `/tmp/b.en.tsv.manifest.json`, which never exists. `manifest` stayed null, `fwd` stayed null, and the provenance-identity refusal was skipped every time.

Resolution is now explicit, first hit wins:

1. An explicit `--manifest`. If given and absent, refuse naming it — an explicit argument is never silently ignored.
2. Derived from the ingest path: `<ingest>.manifest.json`, then the emit form obtained by dropping an `.en` segment sitting immediately before the final extension. This makes `b.tsv` → `b.en.tsv` resolve with no argument.
3. A scan of the ingest file's own directory. Exactly one match is used; zero refuses naming every path tried; two or more refuses as AMBIGUOUS, listing them, and does not guess.

A manifest that fails to parse now refuses naming the parse error — the old code swallowed the throw and continued with a null manifest, the same inert path by another route. A manifest with absent or blank forward provenance refuses too, explaining that the batch was emitted without `--forward-provenance` and that the remedy is to re-emit.

The batch-file existence check moved **before** the manifest resolution, so a mistyped batch path reports a missing file rather than a manifest complaint. The branch printing `(none recorded — the manifest was not found beside the batch)` before carrying on to print triples is deleted: there is no longer any path on which ingest reports a triple without a resolved manifest. Every refusal stays exit 0.

### Task 3 — blinded emit ids (commit `37c9b2ab`)

A live emit wrote `O-Chorus|label|label.rate<TAB>速率`. The key names *are* the English — `label.rate`, `label.depth`, `label.voices` — so for roughly every row the source was handed over in the adjacent column while the header declared it withheld.

`emit()` now draws a per-batch salt from `crypto.randomBytes(16)` and derives each public id as the first 12 hex characters of sha256 over salt + NUL + real id, widening on collision and refusing rather than emitting two rows sharing an id. Rows are sorted by blinded id — the natural emit order groups by plugin then label/title/body, and that structure is itself a hint. The manifest gains `salt`, `blinded: true`, and an id map asserted to cover every row, with a `note` telling the reverse pass to return the id exactly as received and to keep the manifest out of the batch.

`ingest()` joins through that map and refuses when a resolved manifest carries none — a pre-blinding batch must be re-emitted, not joined by guesswork. A new loud line fires when every returned id is unjoinable while at least one was returned: under blinded ids that is the wrong manifest for the batch, not partial drift, and the two are otherwise indistinguishable from a bare `joined: 0` header above an empty list. Triples still print the **real** id.

`research/i18n-zh-hans-localization.md` got the two edits that keep it true: the Z3 row now states the self-mapping exclusion and names the committed generator, and the back-translate paragraph records the blinded ids and the mandatory manifest.

## Positive controls — each observed RED before the fix

These are the point of the task. A plan that records only the green run has recorded nothing.

**Control 1 — `--verify` against the pre-fix committed literal. Exit 1.**

```
gen-zh-trad-only --verify scripts/i18n-zh-lint.js
  characters 4139
  bytes      12773 UTF-8

gen-zh-trad-only: SPOTCHECK FAILED on the committed literal — 5 character(s) on the wrong side:
  像 U+50CF should be OUT of the set but is IN
  乾 U+4E7E should be OUT of the set but is IN
  徵 U+5FB5 should be OUT of the set but is IN
  於 U+65BC should be OUT of the set but is IN
  夥 U+5925 should be OUT of the set but is IN

Refusing to proceed. A set that fails the spotcheck is never written and never accepted.
EXIT=1
```

The 4139 / 12773 figures match the pre-fix provenance block exactly, confirming the reader was reading the real committed literal.

**Control 2 — the self-test with the new Z3 fixtures added but the set not yet regenerated.**

The control fixtures were added *before* the regeneration, deliberately, so the failure could be seen:

```
  SELF-TEST Z3 BROKEN: fires on a control
SELF-TEST: 8/9
```

**Control 3 — pre-fix ingest with `--provenance` byte-identical to the recorded forward pass.**

It should have refused. It printed all 44 triples, after announcing its own inertness:

```
i18n-zh-backtranslate --ingest — en -> zh -> en' triples, worst drift first
  provenance (reverse pass): "fwd-pass-A"
  forward pass recorded at emit: (none recorded — the manifest was not found beside the batch)
  joined: 44

  0.00  O-Chorus|label|label.rate
        en   Rate
        zh   速率
        en'  some english
```

That same line also shows defect D-03 in the raw: `O-Chorus|label|label.rate` in the id column of a batch whose header claims the English is withheld.

**After the fixes**, the same emitted batch reads:

```
0435986bf32c	在干信号与合唱信号之间做平衡。…
04ef76ac61cf	下一个预设
119667d53e13	宽度
```

## Verification

All six items from the plan's verification block, run from the repo root after the final commit:

| # | Check | Result |
|---|-------|--------|
| 1 | `node scripts/gen-zh-trad-only.js --verify scripts/i18n-zh-lint.js` | PASS — 3198 characters / 9660 bytes, exit 0 |
| 2 | `node scripts/i18n-zh-lint.js --self-test` | `SELF-TEST: 9/9` |
| 3 | `node scripts/i18n-zh-lint.js` | 43 plugins, 33 zh entries, Z3 = 0, exit 0 — **unchanged** |
| 4 | `node scripts/i18n-zh-backtranslate.js` | 43 plugins, 44 zh rows (4 mt / 40 bt), exit 0 |
| 5 | The three task verify blocks | all three printed `PASS` |
| 6 | `node scripts/check-i18n.js` | `ALL CHECKS PASS — 43 localized plugin(s)`, exit 0 |

Item 3 is the one that matters most for the Z3 fix: the corpus result must **not** move. This was a latent defect — the corpus holds only 33 zh entries and the O-Chorus pilot has no pan control, so there was never a repo-wide count to watch go to zero. The fixture is the control; the corpus number staying at Z3 = 0 is the proof that nothing else moved.

Beyond the plan's verify blocks, the refusal paths not covered by the automated one-liners were exercised by hand and all print a named reason at exit 0: unparseable manifest, manifest with no recorded forward provenance, explicit `--manifest` naming a nonexistent file, zero candidates, and two candidates (AMBIGUOUS).

The Task 3 human-check was also run mechanically: `grep -cE 'O-|label\.|tip\.|\|'` over the emitted batch returns **0**, and stripping `[0-9a-f]` from the id column leaves **0** characters — the ids are pure lowercase hex and neither the plugin folder name nor any key fragment appears anywhere in the file.

## Deviations from Plan

**None affecting behavior.** One implementation choice worth recording, made under Rule 2 (missing critical functionality):

**[Rule 2 — correctness] `refuse()` generalized rather than duplicated.** The plan asked that all new refusals route through the existing `refuse()` helper "so they keep the tool's report-only exit-0 contract and its established voice." That helper hard-coded a provenance-specific headline (`back-translation provenance is missing or identical to the forward pass`) and a provenance-specific remedy, both of which would have been actively misleading on a manifest-resolution refusal. It now takes an optional `headline` and `remedy`, defaulting to the originals, so the existing call site is unchanged and every new refusal keeps the exit-0 contract with an accurate reason. Committed as part of `9d5cff0b`.

## Known Stubs

None. No stub, TODO, FIXME, skipped test, or unrun `<verify>` was introduced or left behind. Every verify block in the plan was executed and passed.

## Threat Flags

None. No new network endpoint, auth path, file-write path, or schema at a trust boundary was introduced. The three `mitigate` dispositions in the plan's threat register were all implemented:

- **T-f2k-01** (tampering, dictionary fetch) — fetched bytes are never committed and never executed; both sha256 digests are printed and embedded in the provenance block, and the spotcheck assertion stops the write before a wrong set can be committed.
- **T-f2k-02** (information disclosure, `--emit`) — opaque salted row ids, rows sorted by that id, English still withheld. The batch alone identifies neither plugin nor key.
- **T-f2k-03 / T-f2k-04** (spoofing and repudiation, provenance) — the identity refusal now fires because the manifest resolves, and there is no path that reports a triple with unrecorded provenance.

## Scope discipline

No plugin source, no plugin version, and no i18n table was touched. No build was run. No tag was created.

## Self-Check: PASSED

Files:

- `scripts/gen-zh-trad-only.js` — FOUND
- `scripts/i18n-zh-lint.js` — FOUND
- `scripts/i18n-zh-backtranslate.js` — FOUND
- `research/i18n-zh-hans-localization.md` — FOUND

Commits:

- `6da6bd22` — FOUND (Task 1: Z3 generator + corrected derivation)
- `9d5cff0b` — FOUND (Task 2: deterministic manifest resolution)
- `37c9b2ab` — FOUND (Task 3: blinded emit ids)

## Next

Stage 3 — the zh-Hans hard-case wave across the remaining 42 plugins — is now unblocked. The two O-Chorus zh bodies still sitting at `reviewed: 'mt'` remain queued for a reverse batch, which will be the first live exercise of the blinded emit path.
