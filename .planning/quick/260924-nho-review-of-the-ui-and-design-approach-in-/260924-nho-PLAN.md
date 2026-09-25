---
phase: quick-260924-nho
plan: 01
type: execute
wave: 1
depends_on: []
subsystem: ui-review
tags: [ui, design-system, webview, review, analysis, read-only]
autonomous: true

files_modified:
  - .planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md

requirements: [Q-nho-01, Q-nho-02, Q-nho-03, Q-nho-04, Q-nho-05, Q-nho-06, Q-nho-07]

user_setup: []

estimate:
  tokens: 110000
  raw_tokens: 110000
  tasks: 3
  confidence: low        # sample_count=0, factor=1.0 (estimate-calibration, 2026-09-24)

must_haves:
  truths:
    - "The report exists at .planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md and OPENS with an Executive Summary plus the top 3-5 ranked recommendations, before any analysis section (Q-nho-06)."
    - "Appendix A holds a suite-wide census with one row per plugin for ALL 44 plugins, covering BOTH UI roots (36 at plugins/<N>/Source/ui/public, 8 at plugins/<N>/Resources/ui) — per-plugin HTML/CSS/JS line counts, knob family, CSS custom-property count, font stacks, shared-module copies present, resizability, UI test gates present (Q-nho-01, Q-nho-02)."
    - "Duplication is QUANTIFIED, not asserted: for every file that recurs across plugins (e.g. modules/preset-manager.js, webview-drop-streaming.js, js/juce/index.js, i18n.js, tests/ui_tip_render_check.js) the report states copies-count and distinct-content-hash count (Q-nho-01)."
    - "Five analysis sections exist — 1 Architecture, 2 Design System, 3 Pipeline/Process, 4 Quality Gates/Tooling, 5 UX/Accessibility/i18n/Resizing — and every claim in them cites a repo path, a census number, or a reproducible command (Q-nho-01..05)."
    - "Every recommendation carries Impact, Effort, Regression risk (named mechanism, not 'some risk'), the concrete files/paths it would touch, and — where it touches shipped UI runtime code — an explicit reconciliation with the documented 'Per-plugin JS, no shared module' decision (260826-ieq-CONTEXT.md line 51) (Q-nho-06)."
    - "No file under plugins/, modules/, .claude/, scripts/ or .github/ is created or modified by the executor; the working-tree porcelain for those paths matches the start-of-task baseline, and the executor does not commit (Q-nho-07)."
  artifacts:
    - ".planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md — the review: Executive Summary, Ranked Recommendations, Sections 1-5, What Works (keep), Appendix A census, Appendix B method/commands."
    - ".planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-SUMMARY.md — standard quick SUMMARY (orchestrator-owned output)."
  key_links:
    - "Appendix A census is the single source for every number quoted in Sections 1-5 and in the recommendations. A number in a recommendation that is not in Appendix A or reproducible from an Appendix B command is unverifiable."
    - "The census MUST walk both UI roots. Keyed only on Source/ui/public it silently drops 8 plugins — O-Bassoon, O-Bells, O-Bowed, O-FreqPulse, O-Lyrica, O-Reed, O-SpectralShaper, O-Wind — mostly the physical-model instruments, which biases every effect-vs-instrument comparison."
    - "Recommendations that propose shared UI runtime code collide with a recorded decision (260826-ieq-CONTEXT.md:51, memory pattern_no_shared_knob_module_two_families). A recommendation that ignores that decision is not actionable; one that argues to revisit it must say so and be labeled 'Requires decision'."
    - "The visual pass uses scripts/serve-ui.js, which builds each page in an os.tmpdir() copy (serve-ui.js:386) — it is read-only against the repo. Driving it any other way (e.g. pointing a browser at the plugin tree with a hand-injected stub) can write into plugins/."
---

<objective>
Produce an evidence-based review of how UIs are designed, built, and gated across the
44-plugin Ouaricon suite, and recommend ranked, concrete improvements.

Purpose: the user wants to know whether the UI and design approach could be better. The
suite has grown to 44 WebView UIs with no shared knob module, two knob families, hand-copied
tooltip/i18n/test code, and an expanding set of per-plugin and repo-level UI gates. A
measured review tells the user where the real drift and cost are, and what to change first
— without touching any working plugin (regression is this developer's top frustration).

Output: one report, `260924-nho-UI-DESIGN-REVIEW.md`, in the quick directory. Analysis only:
no plugin, module, skill, script, or CI file is edited. The executor does NOT commit — the
orchestrator commits docs.
</objective>

<execution_context>
@~/.claude/gsd-core/workflows/execute-plan.md
@~/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@CLAUDE.md
@.planning/codebase/CONVENTIONS.md
@.planning/codebase/STRUCTURE.md
@.planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-CONTEXT.md
@/Users/taylorbrook/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/pattern_no_shared_knob_module_two_families.md
@/Users/taylorbrook/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/index_ui_gates_and_probes.md
@scripts/check-ui-labels-README.md
@scripts/measure-ui-README.md

Grounded facts (planner-verified at HEAD b7528dc3, 2026-09-24):
- 44 plugin folders under plugins/. UI root is plugins/<N>/Source/ui/public for 36 and
  plugins/<N>/Resources/ui for 8 (O-Bassoon, O-Bells, O-Bowed, O-FreqPulse, O-Lyrica, O-Reed,
  O-SpectralShaper, O-Wind).
- 19 plugins carry Source/ui/public/modules/ (vendored preset-manager.js / webview-drop-streaming.js).
- modules/ui holds only instrument-footer-panel (css/js/snippets) and playable-keyboard (js);
  modules/metering/vu-meter and modules/core/webview-relay-manager are UI-adjacent. Registry: modules/registry.yaml.
- 6 of 44 PluginEditor.cpp files call setResizable.
- Per-plugin UI tests (basename: plugin count): ui_tip_render_check.js 23, ui_tooltip_clamp_check.js 3,
  ui_frontend_check.js 3, ui_preset_menu_check.js 2, ui_layout_check.js 2, ui_shell_diff_check.js 1; ui-stub dirs 11.
- Repo-level UI tooling in scripts/: check-ui-labels.js, check-i18n.js, i18n-fr-lint.js, i18n-zh-lint.js,
  i18n-extract.js, i18n-canon.js, measure-ui.js, boot-all-uis.js, serve-ui.js, ui-stub/.
- CI workflows: .github/workflows/build-and-release.yml, ci-tests.yml — neither mentions check-i18n,
  check-ui-labels, boot-all-uis, or i18n-fr-lint (executor re-verifies and reports).
- Mockup pipeline: .claude/skills/ui-mockup/SKILL.md (360 lines) + references/ (16 files, incl.
  ui-design-rules.md, html-generation.md, aesthetic-integration.md, layout-validation.md);
  .claude/agents/ui-design-agent.md (1291 lines), ui-finalization-agent.md (1244 lines);
  .claude/skills/ui-template-library/SKILL.md (267); .claude/skills/aesthetic-dreaming/SKILL.md (352).
- Aesthetic templates: .claude/aesthetics/ (manifest.json + 5 templates: ouaricon-naturalist-001
  [status official-brand], studio-hardware-001, swiss-minimal-001, vintage-bakelite-001, vintage-hardware-001).
- Mockups exist under plugins/<N>/.planning/mockups for 10 plugins.
- scripts/serve-ui.js exports buildRoot, serve, readEditorSize, resolvePlaywright (module.exports at
  line 543); it copies each page into fs.mkdtempSync(os.tmpdir()) before injecting a stub. Playwright is
  NOT resolvable from the repo root via require.resolve — use S.resolvePlaywright().
- The working tree already carries OTHER sessions' uncommitted edits (modules/persistence/preset-manager/*,
  plugins/O-ReverseDelay/Source/*, plugins/O-Prism/CODE_REVIEW.md, untracked plugins/O-Formant/CODE_REVIEW.md).
  Never touch, stage, or revert them.
</context>

<tasks>

<task type="tracer">
  <name>Task 1 (tracer): Suite-wide UI census, report scaffold, and Section 1 (Architecture) end-to-end</name>
  <files>.planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md</files>
  <read_first>
    - /Users/taylorbrook/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/pattern_no_shared_knob_module_two_families.md (knob Family A/B definitions and reference plugins)
    - .planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-CONTEXT.md lines 45-60 and 130-145 (the per-plugin/no-shared-module decision and its recorded hand-copy-drift risk)
    - modules/registry.yaml (grep -n for ui/, metering/, webview entries only — do not read whole file)
    - modules/ui/instrument-footer-panel/README.md and modules/ui/playable-keyboard/README.md
  </read_first>
  <action>
Proves the whole path raw repo -> measured numbers -> written, cited report section on one section before the rest is built out (Q-nho-01, Q-nho-07).

Step 0 — baseline. Before anything else, record git rev-parse HEAD and the output of git status --porcelain restricted to plugins modules .claude scripts .github into your session scratchpad as nho-baseline.porcelain (absolute scratchpad path from your environment; never /tmp, never inside the repo). All census scripts and intermediate outputs also live in the scratchpad. The ONLY repo file you write in this whole plan is the report.

Step 1 — census. Write a scratch shell or node script that walks all 44 plugin folders and resolves the UI root per plugin: Source/ui/public if present, else Resources/ui; a plugin with neither is reported as "no WebView UI", never dropped. Exclude from authored-code line counts: js/juce/ (the vendored JUCE frontend library), any modules/ subfolder (vendored shared-module copies), img/, fonts, and any bundled/minified file (detect via a sourceMappingURL comment or a single line over 2000 chars; count those separately as "bundled"). Per plugin, collect: UI root; HTML / CSS / authored-JS line counts; knob family (A if the tree contains knob-stem or the ring conic-gradient; B if it contains knob-vine; R if only type="range"; otherwise "unclassified" — do not force a family); count of CSS custom-property definitions (lines matching a --name: declaration); number of distinct font-family values and any bundled font files (count + bytes); presence of vendored copies (modules/preset-manager.js, webview-drop-streaming.js, instrument-footer-panel files, playable-keyboard.js); presence of i18n.js and which languages its table carries (en/fr/zh-Hans); ARIA/keyboard signal counts (role=, aria-, tabindex, keydown); whether plugins/<N>/Source/PluginEditor.cpp calls setResizable and the setSize frame; which tests/ui_*.js gates and ui-stub the plugin has; total UI payload bytes.

Step 2 — duplication. For every file basename that recurs in 3+ plugins (at minimum: modules/preset-manager.js, webview-drop-streaming.js, js/juce/index.js, i18n.js, tests/ui_tip_render_check.js, tests/ui_tooltip_clamp_check.js, ui-stub files, any font binary), report copies-count and distinct md5 count, and for vendored module copies whether each copy's hash matches the canonical source under modules/ (drift count). Also count recurring function/idiom signatures across authored JS (e.g. pointerdown knob drag handlers, getScaledValue readouts, the hover-help toggle id spellings, tooltip show/position functions) to estimate how many hand-copied implementations of each component exist.

Step 3 — write the report with the Write tool. Header: title, "Reviewed at commit <sha>, 2026-09-24", scope line, and a one-paragraph note that Executive Summary and Ranked Recommendations are written last (Task 3) and will sit directly under the header. Then write "## 1. Architecture of UI code" in full: how a UI is structured (index.html + css + app.js + i18n.js + vendored juce lib, the two UI roots and why they differ if discoverable), what is shared via modules/ vs hand-copied, how the module system is used for UI (registry entries vs actual copies, drift found), the two knob families with counts, the recorded no-shared-module decision (cite 260826-ieq-CONTEXT.md:51 and the memory note) and the measured cost of that decision (copies, distinct variants, drift). Then "## Appendix A — Suite census" (one markdown table row per plugin, rows starting with the plugin name so each begins with a pipe then O-, plus the duplication table) and "## Appendix B — Method" (every command/script logic used, so each number is reproducible). Sections 2-5 are appended by Tasks 2-3 between Section 1 and the appendices.

Do not git add or commit anything.
  </action>
  <verify>
    <automated>R=/Users/taylorbrook/Dev/VST-development/.planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md; test -f "$R" && grep -qE '^## 1\. Architecture' "$R" && grep -q '^## Appendix A' "$R" && grep -q '^## Appendix B' "$R" && [ "$(grep -cE '^\| *O-' "$R")" -ge 44 ] && for p in O-Bassoon O-Bells O-Bowed O-FreqPulse O-Lyrica O-Reed O-SpectralShaper O-Wind; do grep -qE "^\| *$p *\|" "$R" || { echo "missing Resources/ui plugin row: $p"; exit 1; }; done && echo TRACER-OK</automated>
  </verify>
  <done>Report file exists with header (commit sha), a complete Section 1 citing census numbers and the 260826-ieq decision, Appendix A with a row for each of the 44 plugins (including all 8 Resources/ui plugins) plus the duplication table with copies/distinct-hash/drift counts, and Appendix B with reproducible method. Baseline porcelain saved in the scratchpad. Nothing committed.</done>
</task>

<task type="auto">
  <name>Task 2: Sections 2 (Design System, incl. visual pass) and 3 (Pipeline/Process)</name>
  <files>.planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md</files>
  <read_first>
    - The report as written by Task 1 (Appendix A numbers are the source for this task's quantitative claims)
    - scripts/serve-ui.js lines 60-80 and 380-470 plus its module.exports block (how to build a served root and read the editor frame size)
    - .claude/aesthetics/manifest.json and .claude/aesthetics/README.md
    - .claude/skills/ui-mockup/SKILL.md, .claude/skills/ui-mockup/BOUNDARIES.md, .claude/skills/ui-mockup/references/ui-design-rules.md, references/html-generation.md, references/aesthetic-integration.md
    - .claude/skills/ui-template-library/SKILL.md and .claude/skills/aesthetic-dreaming/SKILL.md
    - .claude/agents/ui-design-agent.md and .claude/agents/ui-finalization-agent.md — grep -n '^#' first, then read only the sections on component generation, CSS/tokens, knobs/controls, i18n/tooltips, and handoff to implementation
  </read_first>
  <action>
Deep-dive a representative sample and write Sections 2 and 3 (Q-nho-02, Q-nho-03).

Sample (8 plugins, spread across family, category, age, and UI root): O-ReverseDelay (Family A reference, effect), O-Prism (Family B, effect, numeric-entry reference), O-Gain (minimal effect), O-Octagon (complex spatial), O-MicrotonalSampler (complex instrument), O-simpleSubtractive (early simple-series instrument), O-Bells (Resources/ui instrument), O-TextureForge (canvas-heavy). If Appendix A shows one of these is unrepresentative, swap it and say why. Read CSS with targeted ranges: the :root/variables block, the knob/slider/toggle/dropdown/meter/preset-bar/tooltip rules — not whole multi-thousand-line files.

Visual pass (the user treats visual polish as first-class): write a scratch node script in the scratchpad that requires the absolute path /Users/taylorbrook/Dev/VST-development/scripts/serve-ui.js, calls its resolvePlaywright, buildRoot, readEditorSize and serve exports, and screenshots each sampled plugin at its shipping frame into the scratchpad; then view the PNGs with the Read tool. serve-ui builds in an os.tmpdir() copy, so this is read-only against the repo — do not point a browser at the plugin tree directly. If Playwright is unresolvable, state in Section 2 that the visual pass was not performed and why; do not present code-only findings as visual findings.

Write "## 2. Design system consistency": token vocabulary per sample (variable names, naming conventions, how many plugins define a palette via custom properties vs literal hex — from Appendix A), color (palette overlap/divergence, contrast of primary text on primary background computed as WCAG ratio for each sampled plugin), typography (font stacks, bundled font duplication, size scale), spacing/layout (scale or ad-hoc px, grid vs absolute), component inventory (knob, slider, toggle, dropdown, meter, preset bar, tooltip, tabs — how many implementations of each), and whether shipped plugins trace to an .claude/aesthetics template. Be precise about visual observations (element, measured px/color values, which plugin). Cite paths and Appendix A numbers.

Write "## 3. Pipeline and process": how a UI is born (aesthetic-dreaming -> ui-template-library -> ui-mockup -> ui-design-agent -> ui-finalization-agent -> implementation), what each stage emits, and specifically whether the pipeline generates fresh per-plugin component code each time (the root cause of the duplication measured in Section 1) or references the known-good implementations (O-ReverseDelay knob interaction, O-Prism numeric entry). Check whether the pipeline docs know about the requirements that later became mandatory — i18n tables and data-i18n, hover-help layer, tooltip clamp, getScaledValue readouts, UI gates — or whether each plugin retrofits them. Note the 10 mockup dirs under plugins/<N>/.planning/mockups and how many versions they hold. Report size/complexity of the agent docs as a maintenance cost only if you find concrete contradictions or stale guidance (cite line numbers).

Insert both sections after Section 1 and before Appendix A using the Edit tool. Add any new numbers you derived to Appendix A/B so they stay reproducible. Do not modify any file other than the report; do not commit.
  </action>
  <verify>
    <automated>R=/Users/taylorbrook/Dev/VST-development/.planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md; grep -qE '^## 2\. Design system' "$R" && grep -qE '^## 3\. Pipeline' "$R" && awk '/^## 1\. /{a=NR} /^## 2\. /{b=NR} /^## 3\. /{c=NR} /^## Appendix A/{d=NR} END{exit !(a<b && b<c && c<d)}' "$R" && [ "$(grep -oE 'plugins/O-[A-Za-z]+' "$R" | sort -u | wc -l)" -ge 8 ] && [ "$(grep -oE '\.claude/(skills|agents|aesthetics)/[A-Za-z0-9_./-]+' "$R" | sort -u | wc -l)" -ge 5 ] && echo T2-OK</automated>
  </verify>
  <done>Sections 2 and 3 sit between Section 1 and Appendix A, cover tokens/color/typography/spacing/components and the full mockup-to-implementation pipeline, cite at least 8 distinct plugin paths and 5 distinct skill/agent/aesthetic paths, state whether the visual pass ran (with screenshots viewed) or why not, and any new numbers are reproducible via Appendix B.</done>
</task>

<task type="auto">
  <name>Task 3: Sections 4-5, What Works, Executive Summary and ranked recommendations, read-only check</name>
  <files>.planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md</files>
  <read_first>
    - The report as written by Tasks 1-2
    - /Users/taylorbrook/.claude/projects/-Users-taylorbrook-Dev-VST-development/memory/index_ui_gates_and_probes.md (known gate blind spots — use as leads, confirm against the code before citing)
    - scripts/check-ui-labels-README.md, scripts/measure-ui-README.md, scripts/i18n-extract-README.md, scripts/ui-stub/README.md
    - header comment blocks (first ~80 lines) of scripts/check-i18n.js, scripts/boot-all-uis.js, scripts/i18n-fr-lint.js, scripts/i18n-zh-lint.js
    - .github/workflows/ci-tests.yml and .github/workflows/build-and-release.yml (grep for node, ui_, i18n, check- first)
  </read_first>
  <action>
Finish the analysis, then synthesize (Q-nho-04, Q-nho-05, Q-nho-06, Q-nho-07).

Write "## 4. Quality gates and tooling": inventory every repo-level UI tool in scripts/ and every per-plugin tests/ui_*.js gate (counts from Appendix A); a coverage matrix of defect class (label clip/overflow, tooltip clamp, dead selector/binding, i18n completeness, fr/zh lint, preset menu, layout, shell diff, visual regression, accessibility) against the gate that catches it, marking classes nothing catches; which gates run in CI vs only by hand (re-verify by grepping both workflow files); hand-copied gate drift (distinct hashes of per-plugin copies of the same gate from Appendix A); and the cost of the gate estate relative to the UI code it protects.

Write "## 5. UX, accessibility, i18n, resizing": keyboard operability and ARIA coverage across the suite (Appendix A counts; flag plugins with none), focus-visible styling, reduced-motion handling, contrast results from Section 2, hover-help/tooltip approach consistency, i18n coverage (en/fr/zh-Hans per plugin, where tables live, how many plugins lack a language), text-fit strategy (width pins vs fluid layout) and its effect on future languages, and resizing/scaling (6 of 44 resizable — how the other 38 behave on HiDPI or small screens, whether a uniform scale strategy exists).

Write "## What works — keep" listing practices the evidence shows are paying off (so recommendations do not regress them).

Then write, directly under the report header and ABOVE Section 1: "## Executive Summary" (5-8 sentences, the headline findings with their key numbers) followed by "## Ranked recommendations". Each recommendation is an "### R<n>. <title>" heading with these labeled lines: Impact (H/M/L + why, quantified), Effort (S/M/L + rough scope), Regression risk: (the specific mechanism — e.g. which gates or behaviors could change, how many plugins are exposed — and how to contain it, e.g. pilot on one plugin with its existing gates green, then roll out), Paths (concrete files/dirs it would touch), First step (one concrete, reversible action), and Decision note where relevant. Rank by impact/effort; the top 3-5 are named in the Executive Summary. Recommendations that propose shared shipped UI runtime code (e.g. a shared knob or tooltip module) must reconcile with the 260826-ieq-CONTEXT.md:51 decision: either work within it (e.g. canonical reference implementation + drift gate, or pipeline templates emitting the known-good code) or explicitly argue for revisiting it with the measured drift cost, labeled "Requires decision". Pipeline/process and gating recommendations (fixing the root cause of duplication at generation time, CI wiring) should be weighed against refactors of 44 shipped UIs — prefer the ones that stop new drift without touching working plugins, and say so. Also include a short "Not recommended" list for ideas considered and rejected, with the reason.

Final read-only check: compare the current git status --porcelain for plugins modules .claude scripts .github against the scratchpad nho-baseline.porcelain. Any difference must be explained — if you did not write that path it is another session's concurrent work: note it in the SUMMARY and leave it alone (never checkout, restore, or revert). Do not git add or commit; the orchestrator commits the report.
  </action>
  <verify>
    <automated>R=/Users/taylorbrook/Dev/VST-development/.planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md; grep -qE '^## 4\. Quality gates' "$R" && grep -qE '^## 5\. UX' "$R" && grep -q '^## What works' "$R" && awk '/^## Executive Summary/{e=NR} /^## Ranked recommendations/{r=NR} /^## 1\. /{a=NR} /^## 5\. /{f=NR} /^## Appendix A/{d=NR} END{exit !(e && r && e<r && r<a && a<f && f<d)}' "$R" && n=$(grep -cE '^### R[0-9]+\.' "$R") && k=$(grep -c 'Regression risk:' "$R") && p=$(grep -c 'Paths' "$R") && [ "$n" -ge 5 ] && [ "$k" -ge "$n" ] && [ "$p" -ge "$n" ] && grep -q '260826-ieq' "$R" && echo T3-OK</automated>
  </verify>
  <done>Report opens with Executive Summary (headline numbers, top 3-5 recommendations named) and Ranked recommendations (at least 5, each with Impact, Effort, Regression risk mechanism, Paths, First step), followed by Sections 1-5, What works, Appendix A and B. Recommendations touching shared UI runtime code reconcile with 260826-ieq-CONTEXT.md:51 or are labeled "Requires decision". Porcelain for plugins/modules/.claude/scripts/.github matches the baseline or every difference is attributed to another session in the SUMMARY. Nothing committed by the executor.</done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| executor -> shared working tree | Single trunk checkout shared with concurrent sessions (shared HEAD + index); other sessions have uncommitted plugin/module edits in flight |
| scratch scripts -> repo files | Census and screenshot scripts read the repo; the only sanctioned repo write is the report |
| headless browser -> plugin pages | serve-ui.js serves plugin HTML/JS from a temp copy; page JS executes in Playwright |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-nho-01 | Tampering | plugins/, modules/, .claude/, scripts/ working tree | high | mitigate | Read-only commands only; scratch artifacts in the session scratchpad; start/end porcelain baseline diff in Task 1/Task 3; differences attributed, never reverted |
| T-nho-02 | Tampering | git index / commits (concurrent sessions) | high | mitigate | Executor never runs git add/commit/checkout/restore; orchestrator path-scopes the docs commit to the quick directory |
| T-nho-03 | Tampering | visual pass writing stub files into plugin UI trees | medium | mitigate | Screenshots only via scripts/serve-ui.js buildRoot (os.tmpdir() copy, serve-ui.js:386); never serve the live plugin tree with an injected stub |
| T-nho-04 | Information disclosure | report content (licensing/product IDs in UI or editor code) | low | mitigate | Report cites paths and counts, never copies credential-like strings, license keys, or signing identifiers |
| T-nho-05 | Denial of service | executor watchdog on long headless runs | low | mitigate | Screenshot only the 8 sampled plugins (not boot-all-uis across 44); background any command expected to exceed ~5 minutes |
</threat_model>

<verification>
- Report exists at the quick-dir path; section order is Executive Summary, Ranked recommendations, 1-5, What works, Appendix A, Appendix B.
- Appendix A has 44 plugin rows including all 8 Resources/ui plugins; duplication table reports copies and distinct-hash counts.
- At least 5 recommendations, each with Impact, Effort, Regression risk, Paths, First step.
- git status --porcelain for plugins modules .claude scripts .github equals the Task 1 baseline, or every delta is attributed to another session.
- No commit made by the executor.
</verification>

<success_criteria>
The user can read the Executive Summary and top recommendations in under two minutes, trust every number (each traces to Appendix A or an Appendix B command), and pick a first step that stops new UI drift without modifying any working plugin — with the regression exposure of every larger refactor stated up front.
</success_criteria>

<output>
Report: `.planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-UI-DESIGN-REVIEW.md`
Summary: `.planning/quick/260924-nho-review-of-the-ui-and-design-approach-in-/260924-nho-SUMMARY.md`
</output>
