# Requirements: Plugin Freedom System v1.2

**Defined:** 2026-02-04
**Core Value:** Reliable collaborative workflow producing professional-quality plugins — agents use relevant knowledge with full traceability

## v1.2 Requirements

### Resource Discovery

- [x] **DISC-01**: System has a static JSON manifest indexing all research docs with keywords, categories, applicable stages, and summaries
- [x] **DISC-02**: Python discovery script matches task context keywords against manifest entries and returns ranked relevant resources
- [x] **DISC-03**: Discovery filters results by current plugin stage (Stage 0/1/2/3/4) using stage metadata in manifest
- [x] **DISC-04**: Discovery includes agent affinity mapping (dsp-agent gets DSP resources, gui-agent gets UI resources)

### Context Injection

- [x] **INJT-01**: Skill orchestrators inject discovered resource summaries and excerpted content via inject-context.py when stage agents spawn
- [x] **INJT-02**: Skill orchestrators (plugin-workflow, plugin-planning, improve-milestone) add resource section to agent prompts before Task() calls
- [x] **INJT-03**: Stage-specific troubleshooting patterns auto-injected inline by inject-context.py for the corresponding agent without manual configuration
- [x] **INJT-04**: Injected content stays within token budget (max 4,000 tokens of research context per agent)

### Accountability

- [ ] **ACCT-01**: Subagent report JSON schema extended with optional resources_consulted field
- [ ] **ACCT-02**: All stage agents (dsp-agent, gui-agent, foundation-shell-agent, research-planning-agent, polish-agent) updated to report resources consulted
- [ ] **ACCT-03**: SubagentStop hook validates resource usage reporting at warning level (logs gaps, does not block)

### Maintenance

- [ ] **MAINT-01**: Index auto-generation script rebuilds manifest from research/ folder metadata
- [ ] **MAINT-02**: Research docs have YAML frontmatter with created date, last_verified date, and JUCE version
- [ ] **MAINT-03**: Discovery warns when injecting stale resources (last_verified > 90 days)
- [ ] **MAINT-04**: Graceful degradation — agents proceed normally if manifest missing or discovery fails

## Future Requirements (v1.3+)

### Intelligence

- **INTL-01**: Cross-plugin knowledge transfer (reference successful implementations from other plugins)
- **INTL-02**: Decision provenance chains (trace decisions back to source research docs)
- **INTL-03**: Module-research cross-referencing (surface relevant modules alongside research)

### Distribution (Deferred from v1.2)

- **DIST-01**: Windows installer automation (NSIS via GitHub Actions)
- **DIST-02**: Add/Remove Programs integration
- **DIST-03**: Plugin naming standardization

## Out of Scope

| Feature | Reason |
|---------|--------|
| Vector/embedding search | Overkill for 23 documents — static manifest + keyword matching is faster, simpler, and more reliable |
| Full document injection | Burns 5K-50K tokens per doc — context window exhaustion, attention dilution |
| Mandatory enforcement (blocking) | Discovery is heuristic — false positives would block valid work. Warning-level is appropriate |
| Dynamic agent spawning | Current deterministic pipeline is reliable — improve what agents receive, not which agents run |
| Agent-to-agent communication | Claude Code isolates subagents by design — file-based handoffs are the correct pattern |
| Real-time LLM-based routing | Adds latency and cost — keyword matching is faster, free, deterministic |
| Knowledge base web UI | Research is managed via text files and git — no UI needed |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| DISC-01 | Phase 10 | Complete |
| DISC-02 | Phase 10 | Complete |
| DISC-03 | Phase 10 | Complete |
| DISC-04 | Phase 10 | Complete |
| INJT-01 | Phase 11 | Complete |
| INJT-02 | Phase 11 | Complete |
| INJT-03 | Phase 11 | Complete |
| INJT-04 | Phase 11 | Complete |
| ACCT-01 | Phase 12 | Pending |
| ACCT-02 | Phase 12 | Pending |
| ACCT-03 | Phase 12 | Pending |
| MAINT-01 | Phase 13 | Pending |
| MAINT-02 | Phase 13 | Pending |
| MAINT-03 | Phase 13 | Pending |
| MAINT-04 | Phase 13 | Pending |

**Coverage:**
- v1.2 requirements: 15 total
- Mapped to phases: 15
- Unmapped: 0

---
*Requirements defined: 2026-02-04*
*Last updated: 2026-02-05 — INJT-01 through INJT-04 complete (Phase 11)*
