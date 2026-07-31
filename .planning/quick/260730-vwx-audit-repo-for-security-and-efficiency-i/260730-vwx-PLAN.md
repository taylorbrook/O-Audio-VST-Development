---
phase: quick-260730-vwx
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - PUBLIC-RELEASE-READINESS.md
autonomous: true
requirements:
  - QUICK-260730-VWX
must_haves:
  truths:
    - "Opening PUBLIC-RELEASE-READINESS.md at the repo root shows a 3-5 line verdict in the first ~15 lines that states plainly (a) no secret has ever been committed, verified over full history, so no history rewrite is required for security, and (b) the hard blockers are the missing root LICENSE and the undocumented audio samples compiled into shipped plugin binaries."
    - "Every numeric claim in the document matches a figure measured in 260730-vwx-SCOUT.md. No number appears that is not in SCOUT."
    - "Every finding carries a bracketed SCOUT citation, and all 15 SCOUT IDs (S1-S6, L1-L4, E1-E5) appear at least once."
    - "Every irreversible command (history rewrite, visibility flip) is preceded by an explicit gate line and framed as a proposal awaiting the user's decision, never as an instruction already decided."
    - "The ordered checklist sequences any history rewrite LAST among local changes and strictly BEFORE the visibility flip, which is the final gated step."
    - "No repo file other than PUBLIC-RELEASE-READINESS.md is created, modified, moved, or deleted by this plan."
  artifacts:
    - /Users/taylorbrook/Dev/VST-development/PUBLIC-RELEASE-READINESS.md
  key_links:
    - "Each numbered item in the section 6 checklist names the section 2-5 entry (and SCOUT ID) it discharges, so the checklist is navigable back to its evidence."
    - "The JUCE dual-license question (L2) is linked from the LICENSE blocker (L1) as its precondition, because L2 constrains which root license is permissible."
    - "The size-reduction discussion (E1/E2/E5) is linked to the S1 finding so the reader cannot mistake a size cleanup for a security necessity."
---

<objective>
Produce a single decision-ready document, `PUBLIC-RELEASE-READINESS.md` at the repo root, that tells the user exactly what stands between this private repo and a safe public release — separated into hard blockers, security hardening, efficiency/size, and legal decisions — ending in an ordered checklist the user can work through.

Purpose: The user asked what steps are needed to turn the repo public. That is a request for an assessment and a checklist, NOT a request to make the repo public and NOT a request to perform remediation. This plan produces the assessment only.

Output: One new file at `/Users/taylorbrook/Dev/VST-development/PUBLIC-RELEASE-READINESS.md`, committed. Nothing else in the repo changes.
</objective>

<execution_context>
@$HOME/.claude/gsd-core/workflows/execute-plan.md
</execution_context>

<context>
@.planning/quick/260730-vwx-audit-repo-for-security-and-efficiency-i/260730-vwx-SCOUT.md
@CLAUDE.md
</context>

<hard_prohibitions>
This plan is NON-DESTRUCTIVE. The executor writes exactly one new file and commits it.

The executor MUST NOT, under any circumstance:
- change the GitHub repository visibility, or run `gh repo edit` in any form
- run `git filter-repo`, BFG, `git rebase`, `git reset --hard`, or any history-rewriting command
- run `git rm`, `git rm --cached`, or `git mv` on any path
- delete, move, or truncate any existing file
- edit `.gitignore`, `.github/workflows/build-and-release.yml`, or any file under `.claude/` or `.planning/`
- create a root `LICENSE`, `THIRD-PARTY-NOTICES.md`, or any file other than `PUBLIC-RELEASE-READINESS.md`

Every command in the produced document is TEXT FOR THE USER TO READ AND LATER DECIDE ON. The executor documents commands; it does not run them.
</hard_prohibitions>

<source_of_truth>
`.planning/quick/260730-vwx-audit-repo-for-security-and-efficiency-i/260730-vwx-SCOUT.md` is measured ground truth. Read it once, fully, before writing.

Rules:
- Do not re-derive SCOUT's findings. Do not run `du`, `git rev-list`, `git grep`, or any re-measurement. The numbers are already measured.
- Do not contradict SCOUT.
- Do not invent findings. If the document needs a number, it comes from SCOUT.
- If a section feels thin, that is correct — SCOUT found what it found. Do not pad with speculation.
</source_of_truth>

<citation_format>
Every finding in the document cites its SCOUT ID in square brackets, inline, e.g. `... verified over the full history [S1].`

All 15 IDs must appear at least once: `[S1] [S2] [S3] [S4] [S5] [S6] [L1] [L2] [L3] [L4] [E1] [E2] [E3] [E4] [E5]`. This is the traceability gate and is checked automatically.
</citation_format>

<gate_line>
Define one literal gate line, used verbatim, on its own line, immediately ABOVE any fenced code block containing a command that cannot be undone:

`> **IRREVERSIBLE — decide before running.**`

It must appear above (at minimum) the `git filter-repo` proposal and the visibility-flip step. Use it nowhere else, so its presence carries meaning.

Commands that are recoverable but consequential (the `git rm --cached` untracking steps) get a plain one-line note about what they do and do not undo — not the gate line.
</gate_line>

<tasks>

<task type="tracer" tdd="false">
  <name>Task 1: Author PUBLIC-RELEASE-READINESS.md end-to-end</name>
  <files>PUBLIC-RELEASE-READINESS.md</files>
  <read_first>
    - `.planning/quick/260730-vwx-audit-repo-for-security-and-efficiency-i/260730-vwx-SCOUT.md` — read fully, once. It is the only evidence source.
  </read_first>
  <action>
Create `/Users/taylorbrook/Dev/VST-development/PUBLIC-RELEASE-READINESS.md` in a single Write call. Target roughly 250-400 lines. Write all six sections in one pass so the cross-references and the ordering argument stay coherent.

Open with an H1 title and a one-line dateline noting the assessment date and that all figures are measured, citing the SCOUT path.

**Section 1 — Verdict.** Three to five lines, before any other heading, plainly stating the two facts that matter most. First: no secret has ever been committed — no credential-bearing file extensions among tracked files, no credential content patterns in the working tree, none across the full history scan, none among deleted files — therefore no history rewrite is required for security [S1]. Second: the actual hard blockers are the absent root LICENSE [L1] and the undocumented audio samples that are compiled into distributed plugin binaries [L4]. State that everything else is hardening, size, or a disclosure decision. Do not hedge; do not bury either fact.

**Section 2 — Blockers (must fix before going public).** One subsection per item, each with four labelled parts: what, why it matters, the exact command or decision needed, and an effort estimate in the form of a rough time band. Cover exactly four items: the missing root LICENSE and what "no license" legally means for a public repo [L1]; the undocumented sample sets in O-simpleGrain, O-simpleSampler, and the O-MicrotonalSampler 4-layer test fixtures, naming the files and stressing that these ship inside binaries via BinaryData rather than merely sitting in the repo [L4]; `.claude/system-config.json` being tracked despite its `.gitignore` entry, with the explanation that `.gitignore` never untracks an already-tracked file [S3]; and the tracked `build-release/` tree including the compiled `O-Bowed_vst3_helper` executable, noting `.gitignore` covers `build/` but not `build-release/` [E3]. For L1, cross-reference section 5 and state that the JUCE licensing decision [L2] must be settled first because it constrains which root license is even permissible. For L4, present the three resolution paths — locate and document provenance, replace with known-redistributable material, or remove from the binaries — as a decision, not a recommendation.

**Section 3 — Security hardening (should fix).** Cover four items. For the CI workflow [S4], lead with the fact that the current posture is already correct — triggers are push plus workflow_dispatch only, with no pull_request and no pull_request_target, so fork PRs cannot reach the eight Apple signing secrets — and frame the work as preserving that posture and hardening around it. Name the three concrete actions: add a top-level `permissions: contents: read` block since only the release job currently sets one and public repos otherwise inherit the default token scope; SHA-pin the six tag-pinned action references, listing them; and adopt a standing rule to never add `pull_request_target` or a secrets-bearing `pull_request` trigger to this workflow while it carries signing certificates. State the stakes in one line: compromise means signed and notarised malware shipping under the Ouaricon identity. For the local-path disclosure [S2], give the file count and describe the concentration in `.claude/` and `.planning/`, and mark it low severity but high volume. For the internal AI-workflow artifacts [S5], present this as an explicit keep-or-strip decision, with the file counts, and state the tradeoff on both sides — the transparency and documentation value of publishing the planning history versus disclosing agent memory, a session-derived developer profile, and internal verification reports. Do not label it a defect and do not recommend a side. For the tracked build logs [S6], note the count and that they are also `.gitignore`-matched, with the same `git rm --cached` remedy shape as S3.

**Section 4 — Efficiency and repo size.** Reproduce the measured baseline: `.git` size, `.git/objects` size, tracked file count, working tree size [E1 baseline]. Include the largest-blobs table from SCOUT verbatim in structure — size, path, duplicate count — and note the stray scratch render committed into a plugin folder [E1]. Cover the committed installers [E2] and state they belong on GitHub Releases, which the workflow already publishes to. Cover the root-level scratch files and the untracked-but-visible first-impression problem [E4]. Cover clone cost, the GitHub soft-warn threshold and the per-file hard cap, and that the largest file is under the cap so no push will be rejected [E5]. Then state the pivotal mechanic in its own callout: removing files in a new commit shrinks the checkout but NOT `.git`; only a history rewrite reclaims the measured `.git` size; a rewrite changes every commit SHA; therefore it must happen before the repo is public and before anyone forks, or not at all. Present the `git filter-repo` invocation as a proposal the user approves, under the gate line, and note that the `plugins/O-Orbit/libs/SAF` submodule gitlink must survive any rewrite [L3]. Explicitly link back to [S1]: this cleanup is about size, not security, and must not be mistaken for a security necessity.

**Section 5 — Legal and licensing.** Cover all four legal findings with the decision framing. Restate L1 as the gating artifact. Present L2 as a genuine decision the user must make and not one this document makes for them: plugin sources are JUCE-derived, JUCE 8 is dual-licensed AGPLv3 or commercial, `vendored/JUCE-overrides/` redistributes two modified JUCE source files, and JUCE-shipped JS is vendored across many plugins — so the obligations, and therefore the set of permissible root licenses, depend on which JUCE license is held. Lay out what follows under each branch at a high level, and ask the user which license they hold. Cover the third-party attribution inventory [L3] — the SAF submodule with its public HTTPS URL and own license, the bundled JS license file, moodycamel, and the FetchContent dependencies — and recommend an aggregating `THIRD-PARTY-NOTICES.md` as the deliverable. Restate the audio provenance split [L4]: the one documented CC0 folder with its proper provenance note versus the three undocumented sets, and why the binary-shipping distinction raises the risk.

**Section 6 — Ordered execution checklist.** A single numbered list with GitHub-style checkboxes. Each item names the action, the section it discharges, and its SCOUT ID. Order it so that: the licensing decision and root LICENSE come first, because they gate everything; the sample provenance resolution comes next, because it may change which files exist; the untracking and hardening steps follow, because they are cheap and independent; the `.claude/` and `.planning/` keep-or-strip decision resolves before any rewrite, because it changes what the rewrite would need to strip; the history rewrite is LAST among all local changes; and the visibility flip is the final step, under the gate line. Add a short closing note that the rewrite step is genuinely optional — it is a size decision, not a security one — and that skipping it is a legitimate choice.

Formatting rules: use tables where SCOUT used tables. Use inline backticks for paths and commands in prose, and fenced blocks only for multi-line command proposals. Do not use emoji anywhere in the document. Do not use the word "we"; address the user directly. Every finding carries its bracketed SCOUT citation per the citation format above.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && test -f PUBLIC-RELEASE-READINESS.md && for id in S1 S2 S3 S4 S5 S6 L1 L2 L3 L4 E1 E2 E3 E4 E5; do grep -qF "[$id]" PUBLIC-RELEASE-READINESS.md || { echo "FAIL: missing SCOUT citation [$id]"; exit 1; }; done && test "$(grep -cF 'IRREVERSIBLE — decide before running' PUBLIC-RELEASE-READINESS.md)" -ge 2 && test "$(grep -c '^## ' PUBLIC-RELEASE-READINESS.md)" -ge 6 && test "$(wc -l < PUBLIC-RELEASE-READINESS.md)" -ge 200 && echo PASS</automated>
  </verify>
  <done>`PUBLIC-RELEASE-READINESS.md` exists at the repo root with six top-level sections, at least 200 lines, all 15 bracketed SCOUT citations present, and the gate line appearing on at least the two irreversible steps. `git status --porcelain` shows exactly one new untracked file relative to the pre-task state.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: Traceability and safety audit, then commit</name>
  <files>PUBLIC-RELEASE-READINESS.md</files>
  <action>
Re-read the finished `PUBLIC-RELEASE-READINESS.md` against `260730-vwx-SCOUT.md` and correct any drift in place. This is a correction pass on one file, not a rewrite.

Check four things and fix what fails:

1. **Numeric traceability.** Extract every number in the document and confirm each one appears in SCOUT. Pay particular attention to the `.git` and `.git/objects` sizes, the tracked file count, the working-tree size, the local-path file count, the `.claude/` and `.planning/` file counts, the tracked build-log count, the blob sizes and duplicate counts in the size table, the total tracked `.wav` count, the installer sizes and count, the tracked `build-release/` file count, the Apple secret count, the action-reference count, and the GitHub size thresholds. Any number not traceable to SCOUT is either corrected to the SCOUT figure or deleted.

2. **No invented findings.** Every claim maps to a SCOUT finding. Delete anything that does not — including plausible-sounding additions the author may have reached for, such as dependency CVE claims, unmeasured branch or contributor counts, or unmeasured LFS estimates.

3. **Safety framing.** Confirm the executor did not run anything. Confirm every fenced block containing `filter-repo` and every fenced block containing a visibility change is directly preceded by the literal gate line, and is worded as a proposal awaiting the user's approval rather than as a completed decision. Confirm the document nowhere implies the repo has already been made public or that any file has already been removed.

4. **Ordering.** Confirm the section 6 checklist places the history rewrite after every other local change and before the visibility flip, and that the visibility flip is the final item.

Then verify the working tree is otherwise untouched: `git status --porcelain` must show `PUBLIC-RELEASE-READINESS.md` as the only change beyond the pre-existing dirty entries recorded at session start. If any other tracked file shows a modification, stop and report it rather than committing.

Commit only `PUBLIC-RELEASE-READINESS.md` with message `docs(quick-260730-vwx): add public-release readiness assessment and checklist`.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && git log -1 --pretty=format:'%s' | grep -qF 'docs(quick-260730-vwx)' && test "$(git show --pretty=format: --name-only HEAD | grep -c .)" -eq 1 && test "$(git show --pretty=format: --name-only HEAD | grep .)" = "PUBLIC-RELEASE-READINESS.md" && echo PASS</automated>
  </verify>
  <done>The document's numbers all trace to SCOUT, every irreversible command is gated and framed as a proposal, the checklist ordering holds, and exactly one file — `PUBLIC-RELEASE-READINESS.md` — is committed in a single commit whose subject begins `docs(quick-260730-vwx)`.</done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| document → user's shell | Commands printed in the document may be copy-pasted and run by the user with full local privileges |
| SCOUT findings → document claims | Measured evidence crosses into user-facing assertions that drive irreversible decisions |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-vwx-01 | Tampering | `git filter-repo` / visibility-flip commands in the document | high | mitigate | Both are placed under the literal gate line `> **IRREVERSIBLE — decide before running.**`, framed as proposals awaiting user approval, and verified present by an automated `grep -cF` gate in Task 1. Task 2 re-audits the framing. |
| T-vwx-02 | Repudiation | unsourced claims in the document | medium | mitigate | Every finding carries a bracketed SCOUT ID; the Task 1 verify gate enumerates all 15 IDs and fails on any missing. Task 2 performs a numeric traceability pass against SCOUT. |
| T-vwx-03 | Information Disclosure | document content | low | mitigate | The document discusses the local-path and internal-artifact exposure classes but quotes no credential material — SCOUT S1 confirms none exists in the repo to quote. It names tracked paths only, which are already in the repo. |
| T-vwx-04 | Denial of Service | user's repository | high | mitigate | `<hard_prohibitions>` bars the executor from running any history rewrite, `git rm`, file deletion, or visibility change. `files_modified` is a single new file; Task 2 asserts the commit touches exactly that path. |
| T-vwx-SC | Tampering | npm/pip/cargo installs | n/a | accept | This plan performs no package installation. No legitimacy audit is required. |
</threat_model>

<verification>
- `PUBLIC-RELEASE-READINESS.md` exists at the repo root and is committed.
- All 15 SCOUT IDs cited; every number traces to SCOUT.
- The gate line appears on the history-rewrite proposal and the visibility flip.
- The checklist orders the rewrite last among local changes and the visibility flip last overall.
- `git show --stat HEAD` lists exactly one file.
- No history rewrite, no untracking, no deletion, and no visibility change was performed.
</verification>

<success_criteria>
The user can open one file at the repo root and, without reading anything else, know: that the repo has never leaked a secret, what two things genuinely block a public release, which decisions are theirs to make (JUCE license, `.claude/` disclosure, whether to rewrite history at all), and the exact ordered sequence of steps — with commands — to get from here to public.
</success_criteria>

<output>
Create `.planning/quick/260730-vwx-audit-repo-for-security-and-efficiency-i/260730-vwx-SUMMARY.md` when done.
</output>
