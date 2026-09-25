---
phase: quick-260924-thi
plan: 01
subsystem: ui-measurement / brand-template
tags: [R4, contrast, wcag, measure-ui, naturalist-template]
status: complete
requires: [260924-nho UI design review (R4, §2.2, A.4)]
provides:
  - "measure-ui.js --contrast (opt-in, report-only WCAG AA report; per-row `ct`)"
  - "naturalist template AA text variants + 9px text floor + --text-walnut/--text-sage vars"
  - "four-plugin contrast baseline at b2f3d7ba"
affects: [per-plugin R4 colour application (future), ui-template-library apply-operation (new CSS vars)]
tech-stack:
  added: []
  patterns: ["opt-in probe field kept out of --report all for byte-identical default output", "per-screen SKIPPED hint"]
key-files:
  created: []
  modified:
    - scripts/measure-ui.js
    - .claude/aesthetics/ouaricon-naturalist-001/aesthetic.md
    - scripts/measure-ui-README.md
decisions:
  - "Fourth AA variant #4E6839 (sage text on #EBD9C7, 4.54) added — #55703E reads only 4.05 on the template's darker primary paper"
  - "Each variant is bound to its background in a pair table; #715D45 is the walnut safe on both paper tones (4.56 / 5.11)"
  - "--contrast is opt-in and NOT in SCREEN_ORDER; --report contrast is an alias that also turns collection on"
  - "No opaque ancestor -> #FFFFFF canvas base (Chromium default), disclosed; background-image flagged, never sampled"
requirements: [R4]
metrics:
  duration: ~25 min
  completed: 2026-09-25
actuals:
  tokens: 8400     # chars/4 over the realized diff (33,544 chars)
  tasks: 3
  commits: 4       # MEASURED: git rev-list --count b2f3d7ba..HEAD — includes 1 foreign commit (2e45cddc, O-Prism STATUS.md, another session); 3 are this plan's
plan_head_before: b2f3d7bae837b31c61face4ddcb313a169e0ad4f
---

# Phase quick-260924-thi Plan 01: R4 contrast — AA text variants, 9px floor, measure-ui --contrast Summary

Opt-in `measure-ui.js --contrast` gives a per-node WCAG 2.x ratio (composited fg over nearest opaque background) and per-language % below AA / under-9px counts. The naturalist template now names four AA text variants bound to their paper tones and sets a 9px text floor. The baseline is recorded for four plugins, and no plugin was edited.

## Commits

| Task | Commit | Files |
|---|---|---|
| 1 — `--contrast` in measure-ui.js | `e2079e74` | scripts/measure-ui.js |
| 2 — naturalist template AA variants + 9px floor | `3746761b` | .claude/aesthetics/ouaricon-naturalist-001/aesthetic.md |
| 3 — README contrast section, Control 3, baseline | `2b9e77d3` | scripts/measure-ui-README.md |

None of these commits touches `plugins/`. `2e45cddc`, between Tasks 2 and 3, is another session's O-Prism STATUS.md commit and is not part of this plan.

## Baseline — contrast at HEAD `b2f3d7bae837b31c61face4ddcb313a169e0ad4f`

The plugins were measured on a `git archive b2f3d7ba -- plugins/{4 plugins} modules` snapshot, not the working tree, because O-ReverseDelay and O-Prism carry other sessions' uncommitted UI work. Each run was `--contrast --verbose` over every state × every language, keyed by DOM path. All four exited 0.

| Plugin | Frame | Lang | Text nodes | Below AA | % | Under 9px | Over bg-image | Median / min | `step skipped` (verbose) | Review A.4 |
|---|---|---|---|---|---|---|---|---|---|---|
| O-TextureForge | 900×600 | en | 35 | 24 | 68.6% | 10 | 28 | 4.49 / 1.46 | 0 | 69% |
| | | fr | 35 | 24 | 68.6% | 10 | 28 | 4.49 / 1.46 | | |
| | | zh-Hans | 35 | 24 | 68.6% | 10 | 28 | 4.49 / 1.46 | | |
| O-MicrotonalSampler | 900×640 | en | 632 | 106 | 16.8% | 337 | 528 | 10.38 / 2.02 | 3 (1 per lang: `loop-editor-shut`, `#loop-close` not visible) | 44% |
| | | fr | 632 | 106 | 16.8% | 337 | 528 | 10.38 / 2.02 | | |
| | | zh-Hans | 632 | 106 | 16.8% | 337 | 528 | 10.38 / 2.02 | | |
| O-Prism | 1200×800 | en | 652 | 230 | 35.3% | 55 | 1 | 7.44 / 1.36 | 0 | 75% |
| | | fr | 652 | 230 | 35.3% | 55 | 1 | 7.44 / 1.36 | | |
| | | zh-Hans | 652 | 230 | 35.3% | 55 | 1 | 7.44 / 1.36 | | |
| O-ReverseDelay | 940×693 | en | 89 | 8 | 9.0% | 0 | 73 | 7.67 / 1.78 | 0 | 5% |
| | | fr | 89 | 8 | 9.0% | 0 | 73 | 7.67 / 1.78 | | |
| | | zh-Hans | 89 | 8 | 9.0% | 0 | 73 | 7.67 / 1.78 | | |

Totals across all languages (the `contrast: N finding(s)` union of below-AA and under-floor): O-TextureForge 78, O-MicrotonalSampler 1329, O-Prism 777, O-ReverseDelay 24. The aria-hidden share of below-AA nodes is 12 on O-MicrotonalSampler and 0 on the other three. There were 0 transparent and 0 unparsed skips on all four.

**Why these numbers differ from the review's A.4.** This run walks every state in every language and keys each node by DOM path, so panels behind tabs and every sibling count. The review took one default-state pass. That pulls O-Prism (whose many large panels hold mostly passing primary text) and O-MicrotonalSampler down in %, and pushes O-ReverseDelay's small denominator up. O-ReverseDelay's shipping frame is also now 940×693, not the review's 940×768. O-TextureForge, a single-panel page, agrees with the review (68.6% vs 69%).

**Independent cross-check.** `$SCR/xcheck.js` (scratch, never committed) recomputes every ratio from `ct.fg`/`ct.bg` with its own lookup-table luminance, plus `need`, and per language T/B/pct/F/I/median/min from the rows. It then compares these with the parsed stderr lines. The result was `XCHECK OK` on all four: 105 / 1896 / 1956 / 267 counted rows, max |Δratio| = 0.

## Failing-pair worklist (en, top pairs by count) — for R4's per-plugin half

No plugin was edited. This list is the measured before-state for each plugin's future fix.

**O-TextureForge** (19 of 24 below-AA nodes sit over a background-image)
| Count | fg → bg | Ratio | Nodes | Fix |
|---|---|---|---|---|
| 17 | #8B7355 → #FFFFFF (canvas fallback; paper JPG, `img`) | 4.49 | tagline, placeholder-text, section-label, knob-label | `--text-walnut` #7A654B (#715D45 if the paper reads as #EBD9C7). The fallback overstates: on #F5E6D3 this walnut reads 3.66 |
| 2 | #8B7355 → #F5E6D3 | 3.66 | settings-label | #7A654B (4.52) |
| 2 | #DCD5CC → #FFFFFF | 1.46 | fleuron, divider | decoration: add `aria-hidden="true"`; not reading text |
| 1 | #F5E6D3 → #6B8E4E | 3.06 | #gear-btn | green fill → #4E6839 (paper text on it = 5.10) |
| 1 | #F5E6D3 → #5C7A3A | 3.99 | #tips-toggle | same: fill #4E6839 |
| — | 10 nodes at 8px | — | spans | raise to the 9px floor |

**O-MicrotonalSampler** (69 of 106 below-AA nodes sit over a background-image)
| Count | fg → bg | Ratio | Nodes | Fix |
|---|---|---|---|---|
| 33 | #8B7355 → #FAF2E9 | 4.05 | interval-list header/degree/unit, library-item-desc | #7A654B (5.00) |
| 17 | #8B7355 → #F5E6D3 | 3.66 | trigger-precedence-indicator, trim labels | #7A654B (4.52) |
| 11 | #8B7355 → #ECDAC8 | 3.30 | #tuning-readout, knob values | #715D45 (4.60) — #7A654B fails here (4.07) |
| 10 | #8B7355 → #FAF6EE | 4.16 | trigger-hint, th | #7A654B (5.14) |
| 5 | #8B7355 → #F8EBD5 | 3.81 | drop-zone-sep / -or | #7A654B (4.71) |
| — | 337 nodes at 8px (288 `td`, 49 `th`) | — | tuning/interval tables | raise to the 9px floor |

Outside the palette: #A18B7B empty tech-tab (2.64) → #715D45; #B8860B goldenrod active tab/about link on #ECDAC8 (2.39) needs its own darker gold.

**O-Prism** (0 over background-image; every failure is a flat-colour one)
| Count | fg → bg | Ratio | Nodes | Fix |
|---|---|---|---|---|
| 59 | #8B7355 → #EFDFCB | 3.44 | section-header, knob-label | #715D45 (4.80) — #7A654B fails here (4.24) |
| 46 | #A08870 → #EFDFCB | 2.57 | #val-osc* readouts (opacity-reduced walnut) | full-opacity #715D45 (template: no reduced-opacity text) |
| 18 | #CEBCA4 → #F2E3CF | 1.47 | knob-label (dimmed section) | #715D45 (4.97) if the dimming is not a deliberate disabled state |
| 18 | #D5C3AE → #F2E3CF | 1.36 | #val-dist*/#val-chorus* (dimmed section) | same |
| 15 | #8B7355 → #F5E6D3 | 3.66 | section-header, mod-matrix-info, #library-toggle | #7A654B or #715D45 |
| — | 55 nodes at 8px | — | spans, #lbl-glide-mode, #lbl-filt-routing, #tonic-up/down | raise to the 9px floor |

Outside the palette: #4AA4EB flat-deviation blue on #EFDECC (2.05, 10 nodes) needs its own AA blue.

**O-ReverseDelay** (8 of 8 below-AA nodes sit over a background-image)
| Count | fg → bg | Ratio | Nodes | Fix |
|---|---|---|---|---|
| 4 | #C5C1C1 → #FFFFFF | 1.78 | fleuron | decoration: add `aria-hidden="true"` |
| 2 | #B6A79D → #EDE6DE | 1.89 | knob-label | #715D45 (5.06) |
| 2 | #AAA09C → #EDE6DE | 2.07 | #val-driftRate, #val-tukeyTaper | #715D45 |

## Template change (Task 2)

- Four AA variants in a `### Text Contrast (WCAG AA)` pair table, each bound to its background: #7A654B / #F5E6D3 4.52, #715D45 / #EBD9C7 4.56 (5.11 on #F5E6D3), #55703E / #F5E6D3 4.54, and **#4E6839 / #EBD9C7 4.54**.
- **Why #4E6839 was added (planner-derived).** The template names #EBD9C7 as a primary background, and #55703E reads only 4.05 there. Without this row the template would still prescribe a failing sage-text pair. The row is k-scaled from #6B8E4E (k = 0.73), the same method the review used for the other three.
- The originals #8B7355 / #6B8E4E / #8BA870 are kept, now at 17 / 5 / 4 occurrences (up from 16 / 4 / 3), and re-scoped to borders, fills and decoration. Neither the walnut bullet nor the `--brown-border` comment lists text any more.
- Other changes:
  - the 9px text floor under Font Sizing;
  - #D4C4B0 panel text is pointed at #5C4033 (5.51);
  - subtle text uses the variants at full opacity;
  - new `--text-walnut`, `--text-walnut-mid`, `--text-sage` and `--text-sage-mid` variables;
  - two checklist items.
- The `## ` headers are unchanged and `metadata.json` is untouched.

## Verification

- **Task 1.** The fixture positive control printed exactly `contrast: 6 finding(s)` and `en: 13 text node(s), 5 below AA (38.5%), 1 under 9px floor, 1 over background-image, ratio median 3.66 / min 2.69`. All 13 ratios fell within ±0.01, with bg #808080 for i, fg #998B81 for j, a transparent skip for o, and p/q not counted.
- **Determinism note (Step 0).** The pre-edit script was run twice on the O-TextureForge snapshot in both fonts mode and `--mode box --report all`. The two runs were byte-identical in stdout and stderr, with box stdout sha `169a970b0a81…` (matching planning). So byte-identity was the check used, and the post-edit output without `--contrast` passed `cmp`/`diff` against the pre-edit capture in both modes. `--report all` still runs four screens, and `--report contrast --from <rows without ct>` prints `contrast: SKIPPED — needs --contrast (field ct not present)`.
- **Task 2.** The verify passed: every variant recomputes to 4.5:1 or above, and the headers diff is empty.
- **Task 3.** The verify passed: 4 plugins × 3 languages, XCHECK OK ×4, README sections present, and no commit touches `plugins/`. The plan's verify loop over `$SHAS` has to run under bash, because zsh does not word-split, and it passed there.

## Deviations from Plan

1. **[Rule 1 - accuracy] Button-text ratios written as 7.66 / 5.01, not the plan's 7.69 / 5.02.** When the button tints are composited over #F5E6D3 and rounded to 8-bit (the same method the tool uses), #2C3E10 reads 7.66 on #D5D3B5 and 5.01 on #A2B169. Both still pass, and nothing else changed.
2. **HEAD moved since planning** (f9111a53 → b2f3d7ba). The baseline uses the HEAD at execution time, as instructed. O-ReverseDelay's frame is still 940×693.
3. **Extra (additive) changes.** An unparsed *background* colour on the chain also marks the node `'unparsed'`, so the tool never guesses a ratio. `--report` with an unknown name now lists `contrast` among the valid names; that affects only the usage-error path. `CONTRAST_FLOOR_PX` is exported.

## Known Stubs

None.

## Threat Flags

None. No new network, auth or file-access surface. The report runs in the same local headless page as the existing probe.

## Self-Check: PASSED

- FOUND: scripts/measure-ui.js, .claude/aesthetics/ouaricon-naturalist-001/aesthetic.md, scripts/measure-ui-README.md
- FOUND commits: e2079e74, 3746761b, 2b9e77d3
