# Phase 19: Quality Gate Activation - Context

**Gathered:** 2026-03-06
**Status:** Ready for planning

<domain>
## Phase Boundary

Activate 3 dormant hook scripts in the project-level `.claude/settings.json` so they fire on their respective events. The scripts already exist and are fully implemented — this phase is purely about wiring them into settings.json and verifying they fire correctly.

Scripts to activate:
1. `SubagentStop.py` (209 lines) — contract validation after subagent completion
2. `validate-research-frontmatter.py` (272 lines) — YAML frontmatter enforcement for research docs
3. `regenerate-manifest.py` (59 lines) — auto-regeneration of resource-index.json

</domain>

<decisions>
## Implementation Decisions

### Hook failure behavior
- **SubagentStop**: Blocks workflow on contract failures (exit non-zero = error). This is the core purpose of the hook.
- **Frontmatter validator**: Blocks Write/Edit on invalid frontmatter (exit 1). Quality gate enforces compliance at write time.
- **Manifest regeneration**: Never blocks (always exit 0). Index regen is a side-effect, not a gate.
- **Resource accountability** (inside SubagentStop): Warn-only, never blocks. Informational logging only.

### Activation strategy
- All 3 hooks activated in a single settings.json update, one commit. They are independent and don't interact.
- Include smoke test verification after activation — trigger each hook and confirm it fires correctly.
- PyYAML dependency check handled as part of smoke test (not a separate prerequisite task).

### Hook targeting
- **SubagentStop**: No matcher restriction — fires for all subagent completions. The script internally filters to relevant agents (foundation-shell-agent, dsp-agent, gui-agent) for contract validation, while resource accountability runs for all agents.
- **Frontmatter validator**: PostToolUse with `Write|Edit` matcher. The script internally checks if the file is in a `research/` directory and exits 0 if not.
- **Manifest regenerator**: PostToolUse with `Write|Edit` matcher. The script internally checks for research/*.md files.
- **Separate PostToolUse entries** for frontmatter validator and manifest regenerator — each gets its own timeout and independent failure handling.

### Timeout values
- **SubagentStop**: 30 seconds (runs 2-3 subprocess validators sequentially)
- **Frontmatter validator**: 5 seconds (single-file YAML parse)
- **Manifest regenerator**: 10 seconds (scans 54 research docs, writes JSON)

### Path convention
- Use `${CLAUDE_PROJECT_DIR}` in settings.json commands, matching existing hook entries (SessionStart.py, PostToolUse.py, etc.)

### Claude's Discretion
- Whether to reduce SubagentStop's internal subprocess timeout (currently 60s per validator) to align with 30s overall — Claude evaluates compatibility
- How the frontmatter validator receives the file path (stdin JSON parsing vs env var vs argument) — Claude matches existing PostToolUse hook patterns for consistency

</decisions>

<specifics>
## Specific Ideas

- The project-level settings.json is at `.claude/settings.json` (NOT the home directory `~/.claude/settings.json` which contains GSD hooks)
- SubagentStop.py uses `Path(".claude") / "hooks" / "validators"` for relative paths — may need adjustment if CWD isn't project root
- regenerate-manifest.py uses `Path(__file__).resolve().parent.parent.parent` to find project root — this pattern is CWD-independent
- validate-research-frontmatter.py takes file path as `sys.argv[1]` but Claude Code PostToolUse provides tool input as stdin JSON — script may need adaptation

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 19-quality-gate-activation*
*Context gathered: 2026-03-06*
