---
name: improve-review
description: Resolve code-review findings from a plugin's CODE_REVIEW.md
argument-hint: "[PluginName] [findings? e.g. CR-01..02, WR-01..03]"
---

# /improve-review

Resolve the findings in a plugin's `CODE_REVIEW.md` through the `plugin-improve` engine.
`CODE_REVIEW.md` **is** the completed investigation — its root causes and prescribed
fixes are treated as a research handoff (skip re-investigation; verify, then apply).

When user runs `/improve-review [PluginName] [findings?]`, route on argument presence
(see Routing). `CODE_REVIEW.md` lives at `plugins/[PluginName]/CODE_REVIEW.md`.

## Preconditions

<preconditions enforcement="blocking">
  <status_verification target="PLUGINS.md" required="true">
    <check condition="plugin_exists">
      Plugin entry MUST exist in PLUGINS.md.
    </check>
    <check condition="status_in(✅ Working, 📦 Installed)">
      Status MUST be ✅ Working OR 📦 Installed.
      <on_violation status="🚧 In Development">
        REJECT: "[PluginName] is still in development (Stage [N]). Finish it with
        /continue [PluginName] before resolving review findings."
      </on_violation>
      <on_violation status="💡 Ideated">
        REJECT: "[PluginName] is not implemented yet. Use /implement [PluginName] first."
      </on_violation>
      <on_violation status="[any other status]">
        REJECT: "[PluginName] has status '[Status]'. Only ✅ Working or 📦 Installed
        plugins can be review-resolved."
      </on_violation>
    </check>
  </status_verification>
  <review_present target="plugins/[PluginName]/CODE_REVIEW.md" required="true">
    <on_missing>
      REJECT: "No CODE_REVIEW.md found for [PluginName]. Generate one first
      (/gsd-code-review or a review pass), then re-run /improve-review."
    </on_missing>
  </review_present>
</preconditions>

## Routing

**If no plugin name provided:**
1. `ls plugins/*/CODE_REVIEW.md` → build the candidate list.
2. For each, read the front-matter `findings:` block (or the `## Summary`) and show a
   numbered menu: `1. O-Foo (2 critical, 3 warning) — 2026-07-07  2. …`.
3. Wait for selection, then proceed as "plugin name only".

**If plugin name only (no findings filter):**
1. Read `plugins/[PluginName]/CODE_REVIEW.md`.
2. Summarize the findings grouped by severity (CR-* critical, WR-* warning, IN-* info)
   with each finding's one-line title.
3. Recommend a default scope: **all CR-* + WR-*** (Critical + Warning). IN-* are opt-in.
4. Ask which findings to resolve (accept ranges like `CR-01..02, WR-01..03`, `all`, or
   `all + IN-04`). Wait for the answer, then proceed to Resolution.

**If plugin name and findings filter provided:**
1. Parse the filter into an explicit finding-ID set.
2. Proceed directly to Resolution.

## Resolution

<resolution>
  For the selected findings, drive the `plugin-improve` skill as a research handoff so it
  skips Phase 0.5 re-investigation. Invoke via the Skill tool:
    - pluginName: [name]
    - description: "Resolve CODE_REVIEW.md findings [explicit ID list] — CODE_REVIEW.md
      is the completed investigation (root causes + prescribed fixes present)."
    - research_source: plugins/[PluginName]/CODE_REVIEW.md

  <mandatory_steps>
    1. VERIFY-BEFORE-EDIT (do not trust the report blindly). For each finding, read the
       CURRENT source at the cited file:line and confirm the defect still exists. Findings
       may already be partially/fully applied by a prior session or a concurrent process —
       re-check the working tree AND `git diff` before writing. Skip already-fixed ones and
       say so.
    2. VERSION BUMP. Default PATCH (bug fixes). Force MAJOR only if a finding's fix is a
       breaking change (param ID rename/removal, range/type change, state-format change) —
       flag it and confirm before proceeding.
    3. GENUINE FORKS ARE THE USER'S CALL. If a finding has materially different resolutions
       with different risk/scope/version implications (e.g. "remove an inert control" =
       breaking vs "keep + fix the concrete bug" = PATCH vs "implement it properly" = MINOR),
       stop and ask with AskUserQuestion, leading with a recommendation. Do not silently pick.
    4. IN-* ARE OPT-IN. Only resolve Info/Nitpick findings if explicitly selected.
    5. BUILD + VALIDATE. Build via build-automation / build-and-install.sh; on macOS confirm
       `auval -v <type> <subtype> <mfr>` passes before declaring done.
    6. DOCUMENT. CHANGELOG entry (per finding, with root cause), NOTES.md timeline + Known
       Limitations for anything deferred, PLUGINS.md version/date row.
    7. TAG CONVENTION. Tag `O-<Plugin>-v<X.Y.Z>` (NOT a bare `vX.Y.Z` — bare tags collide
       across plugins). Commit + tag only when the user asks; otherwise stage and present
       the ready-to-run commands.
  </mandatory_steps>

  <output_guarantee>
    - Each selected finding either resolved-and-verified, or reported as already-fixed /
      deferred with a reason.
    - Backed up (backups/[Plugin]/v[current]/), built, installed, auval-validated.
    - CHANGELOG / NOTES.md / PLUGINS.md updated; git staged with conventional message +
      `O-<Plugin>-v<X.Y.Z>` tag.
  </output_guarantee>
</resolution>

## Notes

- This is a thin, review-specialized front-end over `plugin-improve`; it reuses that skill's
  backup / version / build / regression / changelog machinery. It only changes the *entry*:
  the findings come from `CODE_REVIEW.md` instead of a free-form description, and Phase 0.5
  investigation is skipped because the review already did it.
- After resolving, the source `CODE_REVIEW.md` can be left in place (as a record) or deleted;
  leave it unless the user asks to remove it.
