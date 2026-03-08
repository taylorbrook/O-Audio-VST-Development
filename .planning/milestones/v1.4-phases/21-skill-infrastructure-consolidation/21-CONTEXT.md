# Phase 21: Skill & Infrastructure Consolidation - Context

**Gathered:** 2026-03-06
**Status:** Ready for planning

<domain>
## Phase Boundary

Merge the overlapping `plugin-phases` skill into `plugin-workflow`, update all references, clean up agent memory placeholders, relocate documentation-only files from `.claude/` root, and exclude dev artifacts from the repo. No new capabilities are being added — this is organizational cleanup.

</domain>

<decisions>
## Implementation Decisions

### Skill merge approach (plugin-phases -> plugin-workflow)
- Keep individual phase commands (`/plugin-discuss`, `/plugin-research`, `/plugin-plan`, `/plugin-execute`, `/plugin-verify`) as standalone entry points within the merged `plugin-workflow` skill
- `/implement` continues to orchestrate the full stage cycle, individual commands provide granular control
- Merge prompts using "best of both" — review both skill's agent prompts for each phase and combine the strongest elements
- Preserve all stage-specific execute agent references (foundation-shell-agent, dsp-agent, gui-agent, polish-agent) in the merged skill
- Inherit `plugin-workflow`'s full directory structure: BOUNDARIES.md, references/, assets/
- Delete `plugin-phases` skill directory after merge is complete
- Update all command and skill file references from `plugin-phases` to `plugin-workflow`

### Agent memory seed patterns
- Populate all 5 agent memory files with known patterns from MEMORY.md, research docs, and project experience
- Include high-impact one-off discoveries (e.g., canvas replaced element gotcha, WebView2 static linking) — not just multi-plugin confirmed patterns
- Augment the existing `research-planning-agent.md` with broader cross-plugin patterns (currently O-Prism-focused)
- Claude curates seed patterns per agent; user reviews and approves the proposed list before writing
- Seed pattern format should match the existing structure in research-planning-agent.md (Learned Patterns, Common Issues, Last Updated)

### File relocation (docs out of .claude/ root)
- Move `agent-profiles.json` to `.claude/references/agent-profiles.json`
- Move `preferences-README.md` to `.claude/references/preferences-README.md`
- Update all references in skill files, hooks, or other code that point to the old locations
- Clean break — no symlinks or backwards-compatibility shims

### Aesthetic HTML cleanup
- Delete all 110 `*ui-test.html` files from git tracking, including files in `backups/`
- Add `*ui-test.html` pattern to `.gitignore`
- Scope the ignore pattern specifically to `*ui-test.html` — don't use broader patterns that could catch legitimate HTML files

### Claude's Discretion
- Exact prompt merging decisions (which parts of each skill's prompts to keep/combine)
- Order of operations for the merge (which files to modify first)
- How to handle any edge cases in reference updates
- Specific seed patterns for each agent (subject to user approval)

</decisions>

<specifics>
## Specific Ideas

- research-planning-agent.md is the gold standard for what a good memory file looks like — specific, actionable patterns with plugin context
- The 4 empty placeholder files all follow the same template ("No patterns recorded yet") — they need real content, not just updated boilerplate
- agent-profiles.json has no runtime effect but documents intended effort levels — it's pure reference documentation
- preferences-README.md at 453 lines documents workflow preferences (express/manual mode, DAW selection)

</specifics>

<deferred>
## Deferred Ideas

- Agent memory write-back mechanism (auto-populating memory from sessions) — Phase 22 (STRC-01)
- Validation cache activation or removal — Phase 22 (STRC-02)

</deferred>

---

*Phase: 21-skill-infrastructure-consolidation*
*Context gathered: 2026-03-06*
