---
quick_id: 260924-nho
status: complete
date: 2026-09-24
type: review
---

# Quick Task 260924-nho — UI and design approach review: Summary

**Deliverable:** `260924-nho-UI-DESIGN-REVIEW.md`. It covers 44 plugins, looks closely at 10 sampled UIs with a headless visual pass through `scripts/serve-ui.js`, and includes Appendix A (census) and Appendix B (method).

**This was analysis only.** No plugin, module, skill, script or CI file was changed. The census read a `git archive b7528dc3` snapshot.

## Execution note
The executor subagent stalled on the 600 s watchdog after it had written Sections 1–5, "What works" and the Appendices. The orchestrator then wrote the Executive Summary, the Ranked recommendations and this SUMMARY directly. Before stating it in the report, the orchestrator re-ran `i18n-zh-lint.js` and confirmed it is red at HEAD (exit 2, O-Comp Z5).

## Top recommendations
1. **R1.** Run the static gates (check-i18n, fr-lint, zh-lint) in CI on push. zh-lint has been red on `main` for 10 days.
2. **R2.** Update the ui-finalization agent and the html-generation references to emit the O-ReverseDelay patterns: the knob, keyboard and ARIA support, `getScaledValue()`, the i18n canon and hover-help.
3. **R3.** Apply canon-as-data drift checks, report-only, to the tooltip, hover-help and knob code, and to the vendored copies of preset-manager and tuning-panel.
4. **R4.** Add AA-passing text colours to the naturalist template, add a contrast report to measure-ui, and fix plugins worst-first.
5. **R5.** Bundle EB Garamond as a module asset.
6. **R6.** Delete the dead `modules/ui/*` or adopt it.
7. **R7.** Add keyboard access and resizing to each plugin when it next goes through `/improve`.

## Working-tree check
Other sessions have uncommitted edits in O-Prism, O-ReverseDelay, O-Formant and preset-manager. This task did not touch any of them.
