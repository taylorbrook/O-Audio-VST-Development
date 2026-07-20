---
phase: quick-260720-akp
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - README.md
autonomous: true
requirements:
  - DOC-README-01
must_haves:
  truths:
    - "README states the current JUCE version (8.0.14), not the stale 8.0.0+ floor."
    - "README reflects cross-platform reality: VST3 on macOS + Windows via CI, AU macOS-only."
    - "README scripts/ structure block lists every script actually present in scripts/."
    - "Every enumerated count in README (templates, phases, requirements) matches the repo."
  artifacts:
    - "README.md (updated in place — targeted edits, not a rewrite)"
  key_links:
    - "JUCE version string consistent across intro, Requirements, and Acknowledgments sections."
    - "Platform claims consistent between the intro line, Prerequisites, and Requirements > Software."
---

<objective>
Review README.md against the current state of the repository and correct stale, missing, or
incorrect content with targeted edits. The README last got a substantive touch in the v1.5
milestone (quick 260427-readme); since then the suite re-based to JUCE 8.0.14, gained a Windows
CI leg, and added build/verification scripts that the README's structure diagram never captured.

Purpose: Keep the public-facing README accurate for the current codebase (39 plugin dirs / 38
registered products, JUCE 8.0.14, cross-platform CI, expanded scripts/).
Output: An updated README.md with factual corrections applied. No rewrite — surgical edits only.
</objective>

<execution_context>
@$HOME/.claude/gsd-core/workflows/execute-plan.md
@$HOME/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@CLAUDE.md
@README.md
@.github/workflows/build-and-release.yml

# Ground-truth findings already established during planning (verify, then apply):
#
# STALE / INCORRECT (fix these):
#   - "JUCE 8.0.0+" appears in Requirements > Software and the framework line. Reality: 8.0.14
#     everywhere — CI has `JUCE_VERSION: '8.0.14'`, local install is pristine 8.0.14 + NE patch.
#   - Intro line 3 frames the system as building plugins "for macOS" only, and Prerequisites /
#     Requirements list only macOS. Reality: build-and-release.yml builds macOS (Universal) AND
#     Windows; scripts/build-and-install.ps1 exists; CLAUDE.md documents a full Windows workflow.
#     AU remains macOS-only; VST3 is cross-platform. The interactive Claude Code dev loop is
#     macOS-primary (auval, AU cache, ~/Library paths) — keep those notes accurate.
#   - Project Structure `scripts/` block (README ~lines 810-812) lists ONLY build-and-install.sh
#     and verify-backup.sh. Actual scripts/ contents:
#       apply-juce-patches.sh, build-and-install.ps1, build-and-install.sh,
#       generate_placeholder_models.py, juce-patches/ (dir), verify-au-link.sh,
#       verify-backup.sh, verify-suite-battery.sh
#   - "17 templates" claim (README ~line 111) is unverified — count the real files.
#
# ACCURATE — DO NOT CHANGE:
#   - "7-phase build system (scripts/build-and-install.sh)" — build-and-install.sh has exactly
#     Phase 1..Phase 7. CLAUDE.md's "Phase 4 dual-variant sweep" == Phase 4 (Remove Old Versions).
#   - v1.5 Milestone History row and "Microtonal Dorico Playback (v1.5)" section.
#   - "25 phases complete across 6 milestones (141 requirements satisfied)" — matches STATE.md.
#   - docs/codebase/ — the directory exists and is correct.
#   - README has no licensing/purchase references (licensing was physically removed in PWYW
#     Path B, quick 260623-bmr) — confirm none crept back in; do not add any.
</context>

<tasks>

<task type="auto">
  <name>Task 1: Audit README claims against repo reality</name>
  <files>README.md</files>
  <action>
    Read README.md in full. Then run the ground-truth checks below and assemble a concrete
    discrepancy list (claim -> reality -> exact README line/section to edit). Do NOT edit yet.

    Verify the planning findings and settle the two open counts:
    - Plugin suite size: `ls -1d plugins/*/ | wc -l` (dirs) and the product-row count in
      PLUGINS.md (`grep -cE '^\| O-' PLUGINS.md`). Note the suite is now much larger than any
      count the README implies; decide whether any README number needs a factual note.
    - Template count: `find .claude/templates -type f | wc -l` and enumerate the category
      subdirectories under `.claude/templates/`. Compare against the README's "17 templates"
      claim and its bulleted category list; record the correct count and categories.
    - JUCE version: confirm `JUCE_VERSION` in .github/workflows/build-and-release.yml and the
      stale minimum-version string(s) in README (framework line + Requirements > Software).
    - Platform: confirm both `build-macos` and a Windows job exist in the workflow, and that
      scripts/build-and-install.ps1 exists.
    - scripts/ block: diff `ls -1 scripts/` against the README Project Structure scripts/ listing.
    - Sanity-grep README for any stale licensing/purchase wording (should find none).

    Produce the discrepancy list in your working notes for Task 2 to consume.
  </action>
  <verify>
    <automated>test -f README.md &amp;&amp; ls scripts/ &amp;&amp; grep -q "JUCE_VERSION: '8.0.14'" .github/workflows/build-and-release.yml &amp;&amp; find .claude/templates -type f | wc -l</automated>
  </verify>
  <done>A discrepancy list exists covering: JUCE version, platform framing, scripts/ structure block, and the template count — each with the exact README section to edit and the corrected value.</done>
</task>

<task type="auto">
  <name>Task 2: Apply targeted README corrections</name>
  <files>README.md</files>
  <action>
    Apply the discrepancy list from Task 1 as surgical Edit operations. Do NOT rewrite sections
    or restructure the document; change only the words that are wrong.

    Required edits:
    1. JUCE version — replace the stale minimum-version references (framework line and
       Requirements > Software) with the current 8.0.14 the CI and local install pin.
    2. Platform framing — update the intro line and the Prerequisites / Requirements > Software
       sections so they reflect cross-platform builds: VST3 on macOS + Windows (via GitHub Actions
       CI, exposed through `/publish`), AU macOS-only. Keep the macOS-primary interactive-dev
       notes (auval, AU cache, `~/Library/Audio/Plug-Ins/`) intact and accurate. Prefer additive
       phrasing over deletion so the macOS-first workflow stays clear.
    3. scripts/ Project Structure block — extend the listing to include every script actually in
       scripts/ (apply-juce-patches.sh, build-and-install.ps1, juce-patches/ dir, verify-au-link.sh,
       verify-suite-battery.sh) with a one-line purpose each, alongside the existing entries.
    4. Template count — correct the "17 templates" number and category bullets to match the
       Task 1 count. If the count is materially different, adjust the surrounding prose too.

    Optional (only if Task 1 confirmed a clear inaccuracy): note the current shipping-suite size
    where the README implies plugin count. Do not invent numbers.

    Leave everything Task 1 marked ACCURATE untouched: the "7-phase" claim, the v1.5 milestone
    content, the "25 phases / 141 requirements" line, and docs/codebase/. Do not add licensing text.
  </action>
  <verify>
    <automated>grep -q "8.0.14" README.md &amp;&amp; grep -q "build-and-install.ps1" README.md &amp;&amp; grep -q "apply-juce-patches.sh" README.md &amp;&amp; grep -q "verify-suite-battery.sh" README.md &amp;&amp; grep -q "7-phase" README.md &amp;&amp; git -C . diff --stat README.md</automated>
  </verify>
  <done>README.md shows 8.0.14 as the JUCE version, lists all current scripts/ entries, reflects cross-platform (macOS + Windows) builds, has an accurate template count, and `git diff --stat` shows a bounded set of line changes to README.md only (no rewrite, no other files touched).</done>
</task>

</tasks>

<threat_model>
Documentation-only change. No trust boundaries are crossed, no code or dependencies are added or
executed, and no package-manager installs occur. STRIDE register is not applicable to a prose
edit of README.md; no threats to enumerate.
</threat_model>

<verification>
- `grep -q "8.0.14" README.md` — current JUCE version stated.
- `grep -q "build-and-install.ps1" README.md && grep -q "apply-juce-patches.sh" README.md && grep -q "verify-suite-battery.sh" README.md` — scripts/ block current.
- `grep -q "7-phase" README.md` — accurate build-pipeline claim preserved.
- `git diff --stat README.md` — only README.md changed; change set is bounded (targeted edits, not a rewrite).
</verification>

<success_criteria>
README.md accurately reflects: JUCE 8.0.14, cross-platform VST3 builds (macOS + Windows via CI)
with AU macOS-only, the complete scripts/ directory contents, and a correct template count —
with the "7-phase build system", v1.5 milestone content, and phase/requirement totals left intact.
Changes are surgical (verified via git diff), touching README.md only.
</success_criteria>

<output>
Create `.planning/quick/260720-akp-review-the-readme-and-update-as-needed/260720-akp-SUMMARY.md` when done.
</output>
