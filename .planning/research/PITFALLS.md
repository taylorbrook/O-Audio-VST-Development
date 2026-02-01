# Pitfalls: Multi-Agent AI Development Systems

**Domain:** AI-assisted VST plugin development workflow
**Researched:** 2026-01-29
**Confidence:** HIGH (verified with multiple authoritative sources including Anthropic engineering, Augment Code, and production failure analyses)
**Focus:** Root causes of quality issues, workflow breakdowns, and rework loops in AI agent systems

---

## Critical Pitfalls

Mistakes that cause rewrites, quality failures, or abandoned work.

---

### Pitfall 1: Specification Ambiguity (41.77% of Multi-Agent Failures)

**What goes wrong:** Agents misinterpret roles, requirements, and constraints. Instead of following clear directives, they explore all possible interpretations of vague prose specifications. The result is code that technically works but misses the actual requirement.

**Why it happens:** Human developers write specifications in natural language that feels clear to them but contains implicit assumptions. "Build a delay plugin" omits tempo sync behavior, feedback limits, and stereo handling. Each AI session interprets these gaps differently, creating inconsistent implementations.

**Warning signs:**
- Different sessions produce structurally different solutions to the same problem
- Agent asks no clarifying questions before diving into implementation
- Output contains features not mentioned in specification (agent filled gaps)
- Rework required because "that's not what I meant"

**Prevention strategy:**
- Convert prose specifications to structured schemas (JSON, YAML) with explicit:
  - **Roles:** What this agent is responsible for (and what it is NOT)
  - **Inputs:** Exact parameters with types, ranges, and defaults
  - **Outputs:** Exact deliverables with acceptance criteria
  - **Constraints:** Performance limits, latency budgets, CPU targets
- Use checklists that force specification completeness before implementation
- Include explicit "out of scope" sections

**Phase to address:** Research/Planning - specifications must be unambiguous BEFORE execution

**Sources:**
- [Augment Code: Why Multi-Agent LLM Systems Fail](https://www.augmentcode.com/guides/why-multi-agent-llm-systems-fail-and-how-to-fix-them)
- [Composio: The 2025 AI Agent Report](https://composio.dev/blog/why-ai-agent-pilots-fail-2026-integration-roadmap)

---

### Pitfall 2: Context Loss Across Sessions

**What goes wrong:** Each session starts from scratch. Previous decisions, architectural rationale, and debugging history vanish. The new session may contradict earlier work, duplicate solved problems, or miss critical context that explains "why" something was built a certain way.

**Why it happens:** LLM sessions are ephemeral by design. Context windows fill up or sessions end. Most AI coding assistants treat each session as a clean slate. Without explicit handoff mechanisms, accumulated knowledge disappears.

**Warning signs:**
- New session contradicts architecture decisions from previous session
- Agent proposes solutions already tried and rejected
- "Lost" debugging progress - same bugs investigated repeatedly
- Code style drift across sessions
- Module dependencies broken because context about shared contracts was lost

**Prevention strategy:**
- Implement structured handoff documents that persist between sessions:
  - **Decisions made:** What was decided and WHY
  - **Blockers encountered:** Problems found and how resolved
  - **Current state:** Exactly where work stopped
  - **Next steps:** What should happen next
- Use STATUS.md files as authoritative state (not conversation history)
- Create CONTEXT.md files per stage that capture architectural decisions
- Log rejected approaches with rationale ("tried X, failed because Y")

**Phase to address:** All phases - context preservation is continuous

**Sources:**
- [Mother CLAUDE: Session Handoffs](https://dev.to/dorothyjb/session-handoffs-giving-your-ai-assistant-memory-that-actually-persists-je9)
- [Amp Handoff Feature](https://ainativedev.io/news/amp-retires-compaction-for-a-cleaner-handoff-in-the-coding-agent-context-race)
- [Anthropic: Multi-Agent Research System](https://www.anthropic.com/engineering/multi-agent-research-system)

---

### Pitfall 3: Coordination Breakdown Between Stages (36.94% of Failures)

**What goes wrong:** Handoffs between stages lose critical information. Stage 2 misinterprets Stage 1's output. Communication breakdowns, state synchronization issues, and conflicting objectives cause "inter-agent misalignment, where one agent's 'complete' response is unusable or confusing to the next."

**Why it happens:** Free-text handoffs lack structure. Each stage interprets the previous stage's output through its own lens. Without explicit contracts and validation, assumptions drift. Research shows that unclear agent-to-agent transfers "drastically increase failure rates."

**Warning signs:**
- Stage N+1 reimplements something Stage N already built
- Output format from one stage doesn't match expected input of next
- "Already done" work discovered mid-stage
- Conflicting assumptions about shared resources (which component owns the filter?)
- Parameters defined differently across stages (range 0-1 vs 0-100)

**Prevention strategy:**
- Define explicit contracts between stages:
  - **Input contract:** What this stage expects from previous
  - **Output contract:** What this stage delivers to next
  - **Validation criteria:** How to verify handoff succeeded
- Use schema-enforced communication (JSON with required fields)
- Implement verification steps at stage boundaries
- Assign clear ownership: "This stage owns X, that stage owns Y"

**Phase to address:** Planning phase defines contracts; each stage boundary validates them

**Sources:**
- [Towards Data Science: How Agent Handoffs Work](https://towardsdatascience.com/how-agent-handoffs-work-in-multi-agent-systems/)
- [Skywork AI: Best Practices for Multi-Agent Orchestration](https://skywork.ai/blog/ai-agent-orchestration-best-practices-handoffs/)
- [Augment Code: Coordination Failures](https://www.augmentcode.com/guides/why-multi-agent-llm-systems-fail-and-how-to-fix-them)

---

### Pitfall 4: Verification Gaps Leading to Cascading Errors

**What goes wrong:** Systems orchestrate workflows without validating that outputs meet requirements. Errors cascade through multiple stages. By the time problems surface, they're deeply embedded and expensive to fix. "21.30% of failures stem from verification gaps."

**Why it happens:** Moving fast feels productive. Verification feels slow. But AI outputs are probabilistic - hallucinations, subtle bugs, and edge case failures are normal. Without validation layers, these compound. "LLMs perform very poorly at detecting issues themselves."

**Warning signs:**
- Tests written AFTER implementation (or never)
- "It compiles" treated as sufficient validation
- Stage verification is optional/skipped when rushed
- No automated checks - verification requires manual DAW testing
- Same bug patterns appear repeatedly across plugins

**Prevention strategy:**
- Independent verification at EVERY stage boundary:
  - Stage output must pass defined acceptance criteria
  - Use automated tests where possible (unit tests, null tests, CPU profiling)
  - Human verification for perceptual quality (does it SOUND right?)
- Deploy "judge" validation (separate prompt/context reviews output):
  - "1.5x to 7x accuracy improvements" from independent validation
- Create verification checklists that block progression until complete
- Build test infrastructure BEFORE DSP implementation

**Phase to address:** Verification infrastructure in Planning; execution at every stage

**Sources:**
- [Augment Code: Verification Gaps](https://www.augmentcode.com/guides/why-multi-agent-llm-systems-fail-and-how-to-fix-them)
- [Addy Osmani: My LLM Coding Workflow 2026](https://addyosmani.com/blog/ai-coding-workflow/)
- [arXiv: Helping LLMs Improve Code Using Testing and Static Analysis](https://arxiv.org/html/2412.14841v1)

---

### Pitfall 5: DSP Quality Degradation (Domain-Specific)

**What goes wrong:** AI-generated DSP code works functionally but sounds amateur. Aliasing artifacts, phase issues, transient smearing, and harsh digital character emerge. Plugin "works" but fails professional quality standards.

**Why it happens:** AI training data lacks real-time-safe audio programming examples. Most professional DSP code is proprietary. "There simply isn't enough training data for LLMs to learn real-time-safe audio programming or audio DSP algorithms." AI interpolates from general programming knowledge, producing code that compiles but doesn't account for audio-specific concerns.

**Warning signs:**
- Sound gets harsher at higher frequencies (aliasing)
- Bass disappears on mono playback (phase cancellation)
- Transients become soft or "mushy"
- CPU spikes on dense material
- "Sounds digital" or "sounds amateur" feedback

**Prevention strategy:**
- Require domain expertise validation for DSP stages:
  - Senior audio developer must review DSP architecture
  - Use reference implementations from research papers
- Build verification tests specific to audio:
  - Aliasing: Sine sweep reveals unexpected high-frequency content
  - Phase: Correlation meter during processing
  - Latency: Measure and report accurately to host
  - CPU: Profile with worst-case material (dense full mix)
- Don't trust AI for novel DSP algorithms - use it for boilerplate, not mathematics
- Test on multiple playback systems (not just studio monitors)

**Phase to address:** DSP Research phase must include algorithm validation; DSP Execution must include audio-specific testing

**Sources:**
- [WolfSound: Don't Use AI for Audio Programming](https://thewolfsound.com/dont-use-ai-for-audio-programming/)
- [KVR Audio: AI-assisted DSP Development Discussion](https://www.kvraudio.com/forum/viewtopic.php?p=9129061)
- ADC 2025: Julian Storer keynote on AI in audio programming

---

### Pitfall 6: Module/Architecture Coupling Across Plugins

**What goes wrong:** Shared modules (UI components, DSP utilities) break when updated. Changes to a shared component for Plugin A break Plugin B. "Like 10 devs worked on it without talking to each other." Inconsistent use of shared code across plugins creates maintenance nightmares.

**Why it happens:** AI treats each prompt in isolation. It doesn't track what's already been written elsewhere. Without explicit architectural constraints, AI creates redundant implementations or modifies shared code without understanding dependencies.

**Warning signs:**
- Same utility implemented differently across plugins
- Updating shared module requires touching multiple plugins
- No clear list of which modules are shared vs plugin-specific
- API contracts change without updating all consumers
- "Works in isolation, breaks in integration"

**Prevention strategy:**
- Explicit module registry documenting:
  - What modules exist
  - Which plugins use each module
  - Version/API contracts
  - Who "owns" the module
- Architectural constraints in specifications:
  - "Use existing WebSliderRelay pattern (don't create new)"
  - "Module X is frozen - do not modify without explicit approval"
- Dependency graphs updated with each change
- Integration tests that verify cross-plugin module compatibility

**Phase to address:** Architecture phase defines module boundaries; Planning enforces them

**Sources:**
- [AIT: How AI-Generated Code Reshapes Architecture](https://ait.inc/tech-stuffs/how-ai-generated-code-is-reshaping-software-architecture/)
- [vFunction: Vibe Coding and Architecture](https://vfunction.com/blog/vibe-coding-architecture-ai-agents/)
- [Addy Osmani: Inconsistency and Duplication Problems](https://addyosmani.com/blog/ai-coding-workflow/)

---

## Moderate Pitfalls

Mistakes that cause delays, technical debt, or user complaints.

---

### Pitfall 7: Infinite Iteration Loops on Bugs

**What goes wrong:** Agent spirals trying to fix a bug, consuming tokens and time without progress. "Nothing shreds your cloud budget faster than an agent that can't decide it's finished." Identical traces repeat in logs. Latency marches upward while solution quality doesn't improve.

**Why it happens:** AI lacks metacognition about when to stop. Self-reflection and self-correction are "the most fragile" parts of agent loops. Without clear exit conditions, the agent keeps trying variations of the same approach. Some reasoning models are "more prominent" in getting stuck in infinite loops.

**Warning signs:**
- Same code pattern tried multiple times with minor variations
- Bug fix introduces new bug, fix for that introduces another
- Token usage spikes without corresponding progress
- Agent "solves" same issue repeatedly in logs
- Hours spent on what should be minutes

**Prevention strategy:**
- Hard limits on iteration attempts (max 3 tries, then escalate)
- Explicit exit conditions in specifications:
  - "If not solved after X attempts, document blocker and stop"
  - "If test still fails after fix, list what was tried and why it failed"
- Different debugging approach after 2 failed attempts (don't repeat same strategy)
- Time-box debugging sessions
- Log rejected approaches to prevent re-trying them

**Phase to address:** Execution phase needs iteration limits; all phases need progress tracking

**Sources:**
- [Galileo: How to Debug AI Agents - 10 Failure Modes](https://galileo.ai/blog/debug-ai-agents)
- [DEV Community: AI Agents Still Struggle with Autonomy](https://dev.to/dataformathub/ai-agents-2025-why-autogpt-and-crewai-still-struggle-with-autonomy-48l0)
- [Getmaxim: Accelerating AI Agent Development](https://www.getmaxim.ai/articles/accelerating-ai-agent-development-best-practices-for-fast-reliable-iteration-in-2025/)

---

### Pitfall 8: UI Polish Degradation

**What goes wrong:** UI looks like "default AI output" - purple/blue gradients, generic layouts, no brand consistency. Components function but lack visual polish. "Large language models struggle to consistently generate UI code that compiles and produces visually relevant designs."

**Why it happens:** LLMs optimize for function over form. UI is multi-signal (layout, spacing, interaction, accessibility, visual intent). Generic training produces generic outputs. Without explicit visual standards, each session produces different aesthetics.

**Warning signs:**
- "Purple design aesthetic" or generic color schemes
- Inconsistent spacing, fonts, or component styling across plugins
- UI "works" but looks unprofessional
- Every plugin looks different (no brand consistency)
- Fine details like shadows, rounded corners, animations are wrong or missing

**Prevention strategy:**
- Explicit design system documentation:
  - Color palette with exact values
  - Typography specifications
  - Component library with examples
  - Spacing/layout rules
- Visual design assets provided (not AI-generated):
  - Brand illustrations
  - Background textures
  - Icon sets
- "Aesthetic checkpoint" before stage completion:
  - Does it match brand guidelines?
  - Is it consistent with other plugins?
- Reference existing plugins as examples, not just abstract specs

**Phase to address:** GUI stage needs design system constraints; verification needs visual review

**Sources:**
- [Addy Osmani: Frontend-Design Skill to End Purple Aesthetic](https://addyosmani.com/blog/ai-coding-workflow/)
- [arXiv: UICoder - Finetuning LLMs for UI Code](https://arxiv.org/html/2406.07739v1)
- [Builder.io: Best LLMs for Coding](https://www.builder.io/blog/best-llms-for-coding)

---

### Pitfall 9: Task Decomposition Failures

**What goes wrong:** Tasks are either too granular (100 tiny tasks that fragment context) or too broad (impossible to verify completion). Planners assign subtasks that are "too granular, too broad, or not serializable."

**Why it happens:** AI doesn't naturally understand optimal task granularity. Without guidance, it either creates a single "build the plugin" task or decomposes into dozens of micro-tasks. Neither supports effective execution.

**Warning signs:**
- Tasks too vague to verify ("improve DSP quality")
- Tasks too granular to track ("change line 47 of file X")
- No clear ordering between tasks
- Dependencies not explicit
- Completion criteria undefined

**Prevention strategy:**
- Task sizing guidelines:
  - Each task completable in single session (2-4 hours work)
  - Each task has testable completion criteria
  - Dependencies explicit in task definition
- Use staged decomposition:
  - Phase -> Stage -> Task hierarchy
  - Validation between stages, not between every task
- Explicit completion criteria:
  - "Complete when: tests pass AND code reviewed AND integrated"

**Phase to address:** Planning phase defines task structure; stage boundaries enforce it

**Sources:**
- [Anthropic: Multi-Agent Research System - Scaling Rules](https://www.anthropic.com/engineering/multi-agent-research-system)
- [n8n: Multi-Agent Systems Tutorial](https://blog.n8n.io/multi-agent-systems/)
- [LangChain: State of Agent Engineering](https://www.langchain.com/state-of-agent-engineering)

---

### Pitfall 10: Cost and Token Explosion

**What goes wrong:** Token usage explodes unexpectedly. "Many teams only notice AI agent pitfalls when the bill arrives." Chatty agents, redundant processing, and context bloat consume resources without proportional value.

**Why it happens:** Multi-agent systems are token-intensive by design (Anthropic reports "about 15x more tokens than chats"). Without efficiency constraints, agents generate verbose responses, pass entire conversation histories between stages, and process same data multiple times.

**Warning signs:**
- Token costs increasing faster than output quality
- Same information processed multiple times by different agents
- Full context passed when summary would suffice
- Expensive reasoning models used for simple tasks
- No cost/quality tradeoff options

**Prevention strategy:**
- Match model to task complexity:
  - Simple lookups: Fast/cheap model
  - Complex reasoning: Full model
  - DSP code: Specialized code model
- Context compression at handoffs:
  - Pass summary, not full history
  - Structured handoff documents, not conversation dumps
- Token budgets per stage:
  - Set limits before execution
  - Alert when approaching budget

**Phase to address:** Planning sets budgets; execution monitors usage

**Sources:**
- [Anthropic: Token Economics in Multi-Agent Systems](https://www.anthropic.com/engineering/multi-agent-research-system)
- [Composio: Cost Explosion](https://composio.dev/blog/why-ai-agent-pilots-fail-2026-integration-roadmap)
- [TechAhead: 7 Ways Multi-Agent AI Fails](https://www.techaheadcorp.com/blog/ways-multi-agent-ai-fails-in-production/)

---

## Minor Pitfalls

Mistakes that cause friction but are recoverable.

---

### Pitfall 11: Over-Confidence Without Verification

**What goes wrong:** AI states incorrect information with full confidence. "Think of an LLM pair programmer as over-confident and prone to mistakes - it writes code with complete conviction, including bugs or nonsense."

**Prevention:**
- Treat AI outputs as proposals, not decisions
- Always verify claims about APIs, library behavior, or system capabilities
- Use Context7 or official docs to validate AI assertions

---

### Pitfall 12: Tool Misuse and Hallucinated Tool Calls

**What goes wrong:** Agent claims to have called tools it didn't, or calls tools with incorrect parameters. "Tools not used but LLM claims they were called - a classic hallucination problem."

**Prevention:**
- Verify tool outputs exist before proceeding
- Log all tool calls for audit
- Structured tool interfaces that validate parameters

---

### Pitfall 13: Compaction Degradation (Long Sessions)

**What goes wrong:** Context summarization ("compaction") during long sessions causes progressive quality decline. "The compaction system was contributing to a gradual decline in performance over time. As sessions accumulated more 'compacts,' accuracy fell."

**Prevention:**
- Prefer fresh sessions with explicit handoffs over compacted sessions
- Break long work into discrete sessions with clear boundaries
- Monitor for quality degradation in extended sessions

---

## Phase-Specific Warnings

| Phase | Likely Pitfall | Mitigation |
|-------|---------------|------------|
| Research | Specification ambiguity | Structured schemas, explicit constraints, out-of-scope sections |
| Planning | Task decomposition failures | Sized tasks with completion criteria, explicit dependencies |
| DSP Development | Quality degradation, iteration loops | Domain expert validation, audio-specific tests, iteration limits |
| GUI Development | UI polish issues, brand inconsistency | Design system constraints, visual checkpoints, reference examples |
| Integration | Module coupling breaks | Module registry, dependency graphs, integration tests |
| Verification | Gaps allowing cascading errors | Independent validation, judge agents, automated test coverage |
| All Phases | Context loss | Structured handoffs, STATUS.md as authoritative state |

---

## Pitfall Checklist: AI-Assisted Development System

### Before ANY Stage Begins:
- [ ] Specification has explicit inputs, outputs, and constraints
- [ ] Previous stage context preserved in structured document
- [ ] Completion criteria defined and testable
- [ ] Iteration limits set (max attempts before escalation)

### During Execution:
- [ ] Progress tracked against defined criteria
- [ ] Rejected approaches logged (don't re-try same strategy)
- [ ] Token usage monitored against budget
- [ ] Breaking changes to shared modules flagged

### At Stage Boundaries:
- [ ] Output validated against acceptance criteria
- [ ] Handoff document captures decisions and rationale
- [ ] Cross-stage contracts verified
- [ ] Quality checkpoint completed (appropriate to stage type)

### For DSP Stages Specifically:
- [ ] Aliasing tested with sine sweeps
- [ ] Phase compatibility verified (correlation meter)
- [ ] Latency measured and reported accurately
- [ ] CPU profiled with worst-case material
- [ ] Tested on multiple playback systems

### For GUI Stages Specifically:
- [ ] Visual consistency with brand design system
- [ ] Pattern consistency with existing plugins
- [ ] Parameter binding verified (bidirectional)
- [ ] Responsiveness verified across window sizes

---

## Root Cause Summary

Based on comprehensive research, the primary causes of quality issues, workflow breakdowns, and rework loops in AI-assisted development systems are:

1. **Specification problems** (41.77%): Vague requirements cause misinterpretation
2. **Coordination failures** (36.94%): Handoff information loss between stages
3. **Verification gaps** (21.30%): Errors cascade without validation layers
4. **Domain expertise gaps**: AI training data insufficient for specialized domains (DSP, professional UI)
5. **Context ephemerality**: Session-based design loses accumulated knowledge
6. **Iteration spirals**: No exit conditions cause infinite debugging loops

**The pattern:** Most failures happen BETWEEN components, not within them. Agent code often works in isolation; integration and handoff is where quality degrades.

**The solution pattern:** Structured contracts, explicit handoffs, independent validation, and domain expert oversight at critical junctures.

---

## Sources Summary

**High Confidence (Official/Engineering Sources):**
- [Anthropic Engineering: Multi-Agent Research System](https://www.anthropic.com/engineering/multi-agent-research-system)
- [Augment Code: Why Multi-Agent LLM Systems Fail](https://www.augmentcode.com/guides/why-multi-agent-llm-systems-fail-and-how-to-fix-them)
- [Addy Osmani: My LLM Coding Workflow 2026](https://addyosmani.com/blog/ai-coding-workflow/)

**Medium Confidence (Industry Analysis):**
- [Composio: 2025 AI Agent Report](https://composio.dev/blog/why-ai-agent-pilots-fail-2026-integration-roadmap)
- [LangChain: State of Agent Engineering](https://www.langchain.com/state-of-agent-engineering)
- [Towards Data Science: How Agent Handoffs Work](https://towardsdatascience.com/how-agent-handoffs-work-in-multi-agent-systems/)

**Domain-Specific (Audio/DSP):**
- [WolfSound: Don't Use AI for Audio Programming](https://thewolfsound.com/dont-use-ai-for-audio-programming/)
- [KVR Audio: AI-Assisted DSP Development](https://www.kvraudio.com/forum/viewtopic.php?p=9129061)

**Academic/Research:**
- [arXiv: A Deep Dive Into LLM Code Generation Mistakes](https://arxiv.org/html/2411.01414v1)
- [arXiv: Helping LLMs Improve Code Using Testing](https://arxiv.org/html/2412.14841v1)

---

---

# Addendum: v1.1 Improvement Pitfalls

**Domain:** Git repository cleanup and agent workflow phase modification
**Researched:** 2026-02-01
**Confidence:** HIGH (verified with official documentation and tool maintainer discussions)

This addendum covers pitfalls specific to the v1.1 milestone improvements: git history cleanup and plugin-improve workflow enhancements.

---

## Git Cleanup Pitfalls

### Pitfall 14: Breaking Existing Clones After History Rewrite

**Impact:** HIGH

**What goes wrong:** After force-pushing rewritten history, collaborators who run `git pull` get cryptic merge conflicts or errors. If they then push without re-cloning, they reintroduce the exact data you tried to remove.

**Warning signs:**
- Any existing clone older than the rewrite
- CI/CD pipelines caching old commit SHAs
- Team members not notified before cleanup
- GitHub Actions caching branches/refs

**Prevention:**
1. Coordinate with all contributors before rewriting
2. Use git-filter-repo's default fresh-clone requirement (it refuses to run otherwise)
3. Document in a pinned issue/announcement that re-cloning is required
4. Schedule cleanup during low-activity periods (weekends, between milestones)
5. After push, verify CI/CD creates fresh clones (most do by default)

**Recovery:** If old history gets pushed back:
- Force push the clean history again
- Explicitly require all collaborators to delete local clones
- Audit who pushed and ensure they understand the process

**Phase assignment:** Phase 1 (Git Cleanup) - must coordinate BEFORE execution

**Sources:**
- [GitHub Docs: Removing Sensitive Data](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/removing-sensitive-data-from-a-repository)
- [git-filter-repo GitHub Repository](https://github.com/newren/git-filter-repo)

---

### Pitfall 15: Remote Origin Removed by git-filter-repo

**Impact:** MEDIUM

**What goes wrong:** git-filter-repo intentionally removes the `origin` remote after rewriting to prevent accidental push to the original repository. Users then can't push or get confused about how to proceed.

**Warning signs:**
- Running git-filter-repo without reading the post-rewrite instructions
- Scripts that assume origin always exists
- CI/CD workflows that depend on origin remote

**Prevention:**
1. Document the expected post-rewrite steps in your runbook
2. Create a simple script that re-adds origin after cleanup:
   ```bash
   git remote add origin <your-repo-url>
   git push --force --branches --tags --prune
   ```
3. Understand this is an intentional safety feature, not a bug

**Recovery:**
```bash
git remote add origin https://github.com/your-org/your-repo.git
```

**Phase assignment:** Phase 1 (Git Cleanup) - document in execution steps

**Sources:**
- [git-filter-repo Issue #46: Remote Removal](https://github.com/newren/git-filter-repo/issues/46)
- [git-filter-repo Man Page](https://www.mankier.com/1/git-filter-repo)

---

### Pitfall 16: BFG Doesn't Clean Latest Commit (HEAD Protection)

**Impact:** MEDIUM

**What goes wrong:** BFG Repo-Cleaner by default protects HEAD - if the problematic files still exist in your latest commit, they won't be removed. Users think cleanup succeeded but the bad files remain.

**Warning signs:**
- Large files or secrets still appear after BFG run
- Protected commit warnings in BFG output
- Running BFG before removing files from current working tree

**Prevention:**
1. Delete/fix problematic files in a normal commit BEFORE running BFG
2. BFG cleans history, not your current state
3. Alternatively use `--no-blob-protection` flag (understand the risks)
4. Use git-filter-repo instead - more predictable behavior

**Recovery:** Delete the files manually, commit, then re-run BFG or use git-filter-repo

**Phase assignment:** Phase 1 (Git Cleanup) - add to pre-cleanup checklist

**Sources:**
- [BFG Repo-Cleaner Documentation](https://rtyley.github.io/bfg-repo-cleaner/)

---

### Pitfall 17: CI/CD Pipeline Failures After Cleanup

**Impact:** HIGH

**What goes wrong:** GitHub Actions, caching, and artifact storage may reference old commit SHAs that no longer exist. Build pipelines fail or behave unexpectedly.

**Warning signs:**
- Workflows that pin specific commits or use commit SHA in caching
- Release workflows that reference historical tags
- Deployment workflows that compare commits
- Any workflow using actions/cache with commit-based keys

**Prevention:**
1. Review all workflow files before cleanup for SHA dependencies
2. Clear GitHub Actions caches after cleanup: Settings > Actions > Caches
3. Test workflows in a fork first with rewritten history
4. Use tag-based or branch-based references where possible

**Recovery:**
- Clear all workflow caches
- Re-run failed workflows
- Update any hardcoded SHAs in workflow files

**Phase assignment:** Phase 1 (Git Cleanup) - add cache clearing to post-cleanup checklist

**Sources:**
- [GitHub Actions Breaking Changes 2025](https://github.blog/changelog/2025-04-15-upcoming-breaking-changes-and-releases-for-github-actions/)
- [OneNine: Reduce Git Repository Size Safely](https://onenine.com/how-to-reduce-git-repository-size-safely/)

---

### Pitfall 18: Repository Size Not Reduced After Cleanup

**Impact:** LOW (annoyance, not data loss)

**What goes wrong:** After removing large files from history, the repository still shows the same size. Users think cleanup failed.

**Warning signs:**
- Clone size unchanged after cleanup
- `.git/objects` directory still large
- GitHub repository size not updated

**Prevention:**
1. Always run `git gc --aggressive --prune=now` after cleanup
2. Wait 24-48 hours for GitHub to run GC on remote
3. GitHub may not show immediate size reduction - this is normal

**Recovery:**
```bash
git reflog expire --expire=now --all
git gc --aggressive --prune=now
# Then force push and wait for GitHub GC
```

**Phase assignment:** Phase 1 (Git Cleanup) - add to post-cleanup verification

**Sources:**
- [Codegenes: Reduce Git Folder Size](https://www.codegenes.net/blog/is-there-a-way-to-reduce-the-size-of-the-git-folder/)

---

### Pitfall 19: Using Deprecated git filter-branch

**Impact:** MEDIUM

**What goes wrong:** git filter-branch is slow, error-prone, and officially deprecated. It can silently corrupt history or produce worse results than you started with.

**Warning signs:**
- Any documentation suggesting `git filter-branch`
- Scripts using filter-branch instead of git-filter-repo
- Long-running cleanup operations (git-filter-repo is 10-720x faster)

**Prevention:**
1. Always use git-filter-repo (officially recommended by Git project)
2. Update any existing cleanup scripts
3. Ignore outdated tutorials suggesting filter-branch

**Recovery:** Start over with git-filter-repo

**Phase assignment:** Phase 1 (Git Cleanup) - enforce tool choice in documentation

**Sources:**
- [Git Tower: git-filter-repo Guide](https://www.git-tower.com/learn/git/faq/git-filter-repo)
- [git-filter-repo GitHub Repository](https://github.com/newren/git-filter-repo)

---

## Workflow Modification Pitfalls

### Pitfall 20: Breaking Existing Plugin-Improve Users with Complexity Creep

**Impact:** HIGH

**What goes wrong:** Adding new phases or flows to `plugin-improve` makes simple fixes (like "tweak this parameter") require understanding a complex multi-phase system. Users who had muscle memory from the current simple flow get frustrated.

**Warning signs:**
- Simple improvements now require multiple phase decisions
- User needs to read documentation for basic operations
- Commands that used to be one-step now require several
- Entry points/commands multiplying

**Prevention:**
1. Preserve the simple path - new phases should be optional/orthogonal
2. Default to current behavior; new phases are opt-in
3. Keep the "quick fix" path as simple as before
4. Add phases for NEW capabilities, don't complicate existing ones

**Recovery:**
- Add express/simple mode that bypasses new phases
- Document clear "I just want to fix X" quick paths

**Phase assignment:** Phase 2 (Workflow Enhancements) - design principle from start

**Sources:**
- [N8N Latest Version 2025](https://latenode.com/blog/low-code-no-code-platforms/n8n-setup-workflows-self-hosting-templates/n8n-latest-version-2025-release-notes-changelog-update-analysis)

---

### Pitfall 21: In-Flight Plugins When Workflow Changes

**Impact:** MEDIUM

**What goes wrong:** Plugins currently in a workflow stage (like O-IntonationPad at Stage 4, O-Freeze at Stage 0) may become incompatible with modified workflow definitions. Stage names change, required fields added, or transitions become invalid.

**Warning signs:**
- Active plugins in registry.json with non-complete status
- Adding required fields to workflow state
- Changing stage names or numbers
- Adding mandatory new phases between existing stages

**Prevention:**
1. New phases should be additive, not modify existing stage definitions
2. Test workflow changes against current in-flight plugins
3. Migration path: complete in-flight plugins before structural changes, OR
4. Grandfather existing plugins: they continue under old rules

**Recovery:**
- Manual migration of STATUS.md files
- Add compatibility shims in workflow code
- Consider workflow versioning (v2 alongside v1)

**Phase assignment:** Phase 2 (Workflow Enhancements) - verify no in-flight plugins OR plan migration

**Sources:**
- [XState: Migrating Running State Charts](https://github.com/statelyai/xstate/discussions/1338)
- Local: registry.json shows O-IntonationPad (Stage 4), O-Freeze (Stage 0) active

---

### Pitfall 22: GitHub Actions Workflow Trigger Compatibility

**Impact:** MEDIUM

**What goes wrong:** Adding new workflow triggers or modifying existing ones can break release automation. The current build-and-release.yml triggers on `*-v*` tags; changes could cause double-triggers, missed triggers, or permission issues.

**Warning signs:**
- Adding new workflow files with overlapping triggers
- Modifying tag patterns
- Not testing workflow changes in a branch first
- GitHub Actions breaking changes (cache service, artifact actions, runner images)

**Prevention:**
1. Test workflow changes in a fork or feature branch first
2. Use `workflow_dispatch` for testing new workflows manually
3. Review GitHub's breaking changes changelog before modifying workflows
4. Current workflow uses `ubuntu-22.04` - safe for now, but monitor deprecation schedule

**Recovery:**
- Revert workflow changes
- Create new release tag to re-trigger

**Phase assignment:** Phase 2 (Workflow Enhancements) - test before merge

**Sources:**
- [GitHub Actions Breaking Changes](https://github.blog/changelog/2025-02-12-notice-of-upcoming-deprecations-and-breaking-changes-for-github-actions/)
- Local: .github/workflows/build-and-release.yml analysis

---

### Pitfall 23: Workflow Schema Drift

**Impact:** LOW-MEDIUM

**What goes wrong:** Adding fields to workflow JSON schemas (active-plugin.json, registry.json) without updating validators or documentation causes silent failures or confusing validation errors.

**Warning signs:**
- Adding fields to JSON files without schema updates
- Schema files referenced in JSON don't match actual structure
- Different agents expecting different schema versions

**Prevention:**
1. Update schema files when adding new fields
2. Make new fields optional with defaults (backwards compatible)
3. Run schema validation in CI
4. Document schema changes in workflow documentation

**Recovery:**
- Add missing fields with sensible defaults
- Update schemas to match actual usage

**Phase assignment:** Phase 2 (Workflow Enhancements) - include schema updates with feature work

**Sources:**
- Local: registry.json references `./schemas/registry.schema.json`
- Local: active-plugin.json references `./schemas/active-plugin.schema.json`

---

## v1.1 Phase-Specific Warnings Summary

| Phase | Likely Pitfall | Mitigation |
|-------|---------------|------------|
| Phase 1: Git Cleanup | Breaking clones, CI cache failures | Coordinate announcement, clear caches post-cleanup |
| Phase 1: Git Cleanup | Using wrong tool (filter-branch) | Enforce git-filter-repo in docs |
| Phase 2: Workflow | Complexity creep in plugin-improve | Design for simple path first |
| Phase 2: Workflow | In-flight plugin incompatibility | Check registry, plan migration OR grandfather |
| Phase 2: Workflow | GitHub Actions breaks | Test in fork, review deprecations |

---

## v1.1 Pre-Implementation Checklist

Before starting v1.1 improvements:

- [ ] Announce planned repository cleanup to any collaborators
- [ ] Verify no time-sensitive CI/CD runs during cleanup window
- [ ] Document in-flight plugins that may be affected by workflow changes
- [ ] Review GitHub Actions deprecation timeline for any affected runners/actions
- [ ] Backup current workflow schemas before modification
- [ ] Plan the "simple path" for plugin-improve before adding complexity

---

*v1.1 Addendum Researched: 2026-02-01*
*Sources: GitHub Docs, git-filter-repo documentation, BFG Repo-Cleaner, GitHub Actions Changelog, XState discussions, local codebase analysis*
