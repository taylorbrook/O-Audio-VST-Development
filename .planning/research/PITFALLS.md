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

---

---

# Addendum: v1.2 Resource Orchestration Pitfalls

**Domain:** Adding resource discovery, injection, and accountability to existing LLM agent system
**Researched:** 2026-02-04
**Confidence:** HIGH (grounded in direct codebase inspection + external research on LLM context management, JSON Schema evolution, and RAG retrieval pitfalls)

This addendum covers pitfalls specific to the v1.2 milestone: adding agent intelligence and resource orchestration to the Plugin Freedom System. These pitfalls are about what goes wrong when you ADD resource orchestration to an EXISTING agent system with 13 agents, 6 hooks, and JSON Schema contracts.

**System measurements at time of research:**
- 13 agents (11 .md files in `.claude/agents/`), 25 skills, 6 hooks
- 23 research documents in `research/` (118-2,336 lines each, 8KB-72KB)
- Agent prompts range from 126-1,301 lines (dsp-agent.md is 1,235 lines alone)
- Hook timeouts: 2s (PostToolUse), 5s (SessionStart, UserPromptSubmit, Stop), 10s (SubagentStop, PreCompact)
- JSON Schema draft 2020-12 with `additionalProperties: false` on all contracts
- Subagents spawned via Task tool with isolated 200K-token context windows

---

## Critical Pitfalls (v1.2)

### Pitfall 24: Context Window Budget Exhaustion from Research Doc Injection

**What goes wrong:**
Injecting research documents into agent prompts pushes total input tokens past the effective performance threshold. The dsp-agent prompt is already 1,235 lines (~25K tokens). Adding even one research doc like `reverb-comprehensive-research.md` (1,580 lines, ~44KB, ~11K tokens) would increase the agent's input by 44%. Injecting 2-3 relevant docs could push a single agent invocation to 60-80K tokens before the agent reads any project files or contracts (ARCHITECTURE.md, parameter-spec.md, ROADMAP.md). Research shows LLM performance degrades non-linearly -- the NoLiMa benchmark found that at 32K tokens, 11 out of 12 tested models dropped below 50% of their short-context performance. Chroma's context rot study measured 18 LLMs and found "models do not use their context uniformly; instead, their performance grows increasingly unreliable as input length grows." Claude's 200K context window is theoretical capacity, not effective capacity.

**Why it happens:**
The intuition "more context = better output" is wrong for LLMs. Developers assume giving the agent all relevant research will help it make better decisions. In reality, the agent's attention mechanism struggles to find relevant information within large inputs, especially content buried in the middle ("lost in the middle" phenomenon, confirmed by Liu et al. and updated by Veseli et al. 2025). Additionally, research documents were written for human consumption with full prose, tables, and code examples -- they contain far more tokens per actionable insight than a targeted injection needs.

**Warning signs:**
- Agent output quality decreases after adding resource injection (more generic, less focused)
- Agent stops referencing specific details from research docs despite having them in context
- Agent responses become longer but less actionable (padding to seem thorough)
- Context compaction triggers more frequently during agent workflows
- Token costs per plugin build increase significantly without quality improvement
- Agent misses contract requirements (ARCHITECTURE.md, parameter-spec.md) because research docs consumed the attention budget

**How to avoid:**
1. **Budget per agent:** Set hard token limits for injected resources. A practical ceiling is 2,000-4,000 tokens of injected research per agent invocation (~10-20 focused paragraphs). This keeps total agent input under 40K tokens where performance is strongest.
2. **Extract, don't inject whole docs:** Instead of injecting full research documents, extract relevant sections. If dsp-agent is building a reverb, inject only the "JUCE Implementation" and "Core DSP Building Blocks" sections from reverb-comprehensive-research.md (~200 lines), not the entire 1,580-line document.
3. **Summarize for injection:** Create condensed "agent-ready" summaries (200-500 tokens) of each research doc. The full doc stays available for the agent to Read tool into if it needs depth.
4. **Tiered injection:** Level 1 = summary pointer (50 tokens: "reverb-comprehensive-research.md available -- covers Schroeder, Freeverb, FDN, Dattorro, JUCE integration"). Level 2 = key sections (500 tokens of most relevant content). Level 3 = agent reads full doc via Read tool on demand. Start with Level 1 only.

**Phase to address:** Phase 1 (resource discovery design). The injection strategy must be designed before any injection code is written. Getting this wrong means every subsequent phase inherits the problem.

**Confidence:** HIGH -- based on published research (Chroma context rot study, NoLiMa benchmark, Claude API docs), cross-referenced with actual measured sizes of this system's research docs and agent prompts.

---

### Pitfall 25: False Relevance from DSP Keyword Ambiguity

**What goes wrong:**
A keyword-based resource discovery system returns wrong documents because audio/DSP terminology is inherently ambiguous. Concrete examples from this codebase:

| Search Term | Intended Doc | False Positive |
|-------------|-------------|----------------|
| "frequency" | fft-processing-best-practices.md | Every single research doc (frequency is universal in audio) |
| "filter" | delay-effects-comprehensive-guide.md (delay filter section) | fft-artifact-prevention.md (window functions), circuit-modeling-fundamentals.md (analog filters) |
| "modulation" | generative-audio-algorithms-reference.md | microtonality docs (frequency modulation in tuning context) |
| "spectral" | spectral-sequencer-research.md | spectral-transient-shaper-research.md, spectral-toolbox-synopses.md (3 docs share the word) |
| "buffer" | Any DSP doc | webgl-spectrogram-patterns.md (WebGL buffer), dsp-click-prevention-debugging.md |
| "envelope" | multi-stage-decay-envelopes-comparison.md | modal-synthesis-bells-academic-research.md, generative-audio-algorithms-reference.md |
| "phase" | fft-processing-best-practices.md (phase vocoder) | Every doc (phase is used in "phase of development", "phase accumulator", "phase response") |

The problem is acute because all 23 research docs are in the same narrow domain (audio DSP). Traditional RAG false positive rates are already concerning; in a corpus where every document shares core vocabulary, simple keyword matching is nearly useless for discrimination.

**Why it happens:**
Audio DSP is a domain where core concepts (frequency, amplitude, phase, buffer, sample, filter, envelope, modulation) appear in virtually every document. Unlike a general knowledge base where "frequency" might disambiguate between audio and radio topics, all 23 docs are audio-focused. Keyword matching cannot distinguish between "FFT frequency bin manipulation" and "filter cutoff frequency parameter" because the word "frequency" carries different meanings at different specificity levels within the same domain.

**Warning signs:**
- Discovery returns 5+ documents for every query (should return 1-3 for focused tasks)
- Agents receive research docs they never reference in their output
- The same documents keep getting injected regardless of the specific plugin being built
- Discovery returns spectral-toolbox-synopses.md (a short overview) when agent needs spectral-sequencer-research.md (the deep dive)

**How to avoid:**
1. **Use structured metadata, not content search:** Each research doc should have YAML frontmatter with explicit tags: `topics: [reverb, algorithmic-reverb, fdn, dattorro, juce-reverb]`, `plugin_types: [effect, reverb]`, `dsp_techniques: [allpass, comb-filter, delay-network]`. Match against these tags, not document content.
2. **Match on plugin BRIEF.md, not generic keywords:** The creative brief already describes what the plugin does. Match brief terms like "algorithmic reverb with Dattorro topology" against doc metadata, not raw keywords.
3. **Human-curated relevance over automated matching:** With only 23 docs, the research corpus is small enough to curate. A static lookup table mapping DSP technique to relevant docs is more reliable than any automated search:
   ```
   reverb -> [reverb-comprehensive-research.md]
   fft/spectral -> [fft-processing-best-practices.md, fft-artifact-prevention.md, custom-fft-implementations.md]
   delay -> [delay-effects-comprehensive-guide.md]
   ```
4. **Two-stage matching:** First match on high-level topic (e.g., "reverb"), then if multiple docs match, use the plugin's specific technique (e.g., "Dattorro topology") to narrow further.

**Phase to address:** Phase 1 (design tagging scheme) + Phase 2 (build manifest with tags). The scheme must be decided before any matching code is written.

**Confidence:** HIGH -- directly verified by inspecting all 23 research doc filenames and content headers, confirmed by RAG false positive research.

---

### Pitfall 26: Breaking Existing Contracts with `additionalProperties: false`

**What goes wrong:**
The system uses `additionalProperties: false` on JSON Schema contracts (confirmed in `subagent-report.json` and documented in agent-contracts README). Adding resource orchestration requires new fields in agent input/output contracts (e.g., `resources_injected`, `resources_consulted`, `discovery_metadata`). With `additionalProperties: false`, adding ANY new field to a schema causes validation failures for ALL existing producers that don't include that field.

This is particularly dangerous because `subagent-report.json` is used by 6 agents simultaneously (research-planning-agent, foundation-shell-agent, dsp-agent, gui-agent, ui-design-agent, ui-finalization-agent). Updating the schema without updating ALL 6 agents will cause validation failures. The SubagentStop hook runs `validate-checksums.py` and `validate-cross-contract.py` which parse these schemas -- schema changes may break validators too.

**Why it happens:**
`additionalProperties: false` was the correct choice for v1.0 -- it prevents typos and unknown fields from silently corrupting data. But it creates a "closed content model" that resists evolution. JSON Schema community research (Creek Service) confirms: in closed models, adding new optional properties is backward compatible (old schemas can read new data), but NOT forward compatible (old schemas reject data with new fields). Since validators run the SCHEMA against AGENT OUTPUT, updating the schema first means old agents fail, and updating agents first means the old schema rejects the new fields. Both directions break.

**Warning signs:**
- Agents fail with "Additional properties not allowed" validation errors after schema update
- SubagentStop hook rejects valid agent completions
- Some agents work (updated ones) while others fail (not yet updated) -- partial rollout breakage
- Validators crash or return false negatives after schema changes

**How to avoid:**
1. **Add new fields as optional with defaults:** `"resources_consulted": {"type": "array", "default": [], "description": "..."}` -- NOT added to the `required` array. Old agents that don't produce the field still pass because the default applies.
2. **Version bump using existing CHANGELOG.md process:** Adding optional fields = MINOR bump (v1.0.0 -> v1.1.0). The system already has this process documented.
3. **Atomic updates:** Update ALL 6 agent prompts + schema + validators in the same commit. Never deploy a schema change without updating all consumers.
4. **Consider a separate schema:** Instead of extending `subagent-report.json`, create a new `resource-usage-report.json`. The orchestrator collects both reports from each agent. Zero risk to existing contracts.
5. **Test with sample data first:** Use the Python `jsonschema` approach already documented in schemas/README.md to verify backward compatibility before touching any agent prompt.

**Phase to address:** Phase 1 (contract evolution planning). Must be decided before any code changes.

**Confidence:** HIGH -- directly verified: `subagent-report.json` has `additionalProperties: false` (line 162), agent-contracts README documents strict validation, CHANGELOG.md shows v1.0.0 with no prior evolution events.

---

### Pitfall 27: Resource Discovery Becoming a Single Point of Failure

**What goes wrong:**
Every agent invocation now depends on a discovery step. If discovery fails (bug in matching logic, malformed manifest, missing metadata), the agent either: (a) runs without resources (defeating the purpose), or (b) fails entirely (blocking the plugin build workflow). Either outcome is worse than the current state where agents simply don't have resources but work fine.

The risk is amplified by the existing hook architecture. If resource discovery runs in a hook, a failure can either silently exit with code 0 (agent runs without resources, nobody notices) or block the workflow with a non-zero exit (agent can't run at all). Neither is acceptable.

**Why it happens:**
Resource orchestration introduces two new dependency steps into a working pipeline:
```
Current:  User prompt -> Agent prompt -> Agent reads contracts -> Agent executes
Proposed: User prompt -> Discovery -> Injection -> Agent prompt -> Agent reads contracts -> Agent executes
```
The system currently has no precedent for pre-agent context injection that affects prompt construction. `UserPromptSubmit.sh` only injects for `/continue` commands, not for all agent invocations. The infrastructure for reliable pre-agent injection does not exist yet.

**Warning signs:**
- Agents occasionally run without resources despite the system being "configured" (silent failures)
- A bug in the manifest/index blocks ALL plugin builds, not just the one being worked on
- Discovery returns empty results for valid queries, causing agents to produce generic output
- Error messages don't distinguish "discovery found nothing relevant" from "discovery system crashed"

**How to avoid:**
1. **Graceful degradation, not hard failure:** If discovery fails or returns empty, the agent runs with its existing prompt (current behavior). Resource injection is purely additive. Log a warning but never block the workflow due to discovery failure.
2. **Separate discovery from injection:** Discovery produces a list of relevant resource paths. Injection decides what to include. Each step fails independently and gracefully.
3. **Static fallback over dynamic discovery:** For v1.2, use a static mapping (plugin DSP technique -> relevant research docs) rather than dynamic content analysis. A JSON lookup table cannot crash. Dynamic discovery can be layered on in v1.3+.
4. **Discovery health check in SessionStart.sh:** Verify the resource manifest is valid and loadable at session start. Catch problems before they affect agent invocations.
5. **Orchestrator-level discovery (preferred):** Have the plugin-workflow skill run discovery before spawning each subagent, passing results as part of the Task prompt. The skill has no timeout constraint and full error handling capabilities.

**Phase to address:** Phase 2 (implementation). Graceful degradation must be the default from day one.

**Confidence:** HIGH -- based on hook architecture inspection (timeout constraints, exit code behavior) and the absence of pre-agent injection infrastructure.

---

## Moderate Pitfalls (v1.2)

### Pitfall 28: Hook Timeout Violations During Resource Discovery

**What goes wrong:**
Resource discovery logic added to hooks exceeds the configured timeout, causing silent failure or workflow blocking. Measured current timeout budget:

| Hook | Timeout | Estimated Current Usage | Available Budget |
|------|---------|------------------------|-----------------|
| SessionStart.sh | 5,000ms | ~2,000ms (env checks) | ~3,000ms |
| UserPromptSubmit.sh | 5,000ms | ~500ms (cat files) | ~4,500ms |
| PostToolUse.sh | 2,000ms | ~1,500ms (regex + python validator) | ~500ms |
| SubagentStop.sh | 10,000ms | ~3,000ms (3 python validators) | ~7,000ms |
| PreCompact.sh | 10,000ms | ~2,000ms (cat contracts) | ~8,000ms |

Discovery involves: reading a manifest, matching against task context, reading matched doc metadata, formatting injection payload. With file I/O for 3-5 research doc headers, this takes 1-3 seconds. Adding this to PostToolUse.sh (500ms available) WILL cause timeout failures. Even UserPromptSubmit.sh (4.5s available) is tight if discovery reads multiple files on cold disk cache.

**Why it happens:**
Hook timeouts were set for validation operations (checksums, regex, parameter checking), not for I/O-intensive discovery across a corpus of 23 files. The existing validators are fast because they operate on predictable, small inputs. Resource discovery operates on variable-size corpus with file I/O that depends on disk cache state.

**Warning signs:**
- Hooks intermittently time out (works on warm cache, fails on cold)
- PostToolUse.sh starts failing consistently after adding discovery logic
- Discovery works during development (testing with 2-3 files) but fails with full corpus (23 files)

**How to avoid:**
1. **Do NOT add discovery to PostToolUse.sh.** The 2s timeout with ~500ms available is not viable. This hook must remain focused on code validation only.
2. **Pre-compute discovery results at workflow start** in the orchestrator skill (no timeout constraint). Cache results. Pass to agents as part of Task prompt.
3. **If hooks must be used, use SessionStart.sh:** Build a cached index at session start (3s available). Store as `.claude/cache/resource-index.json`. Subsequent operations just read the cache (~50ms).
4. **Benchmark independently before deploying:** `time python3 .claude/scripts/resource-discovery.py --plugin O-Reverb --stage 2`. If it exceeds 1 second, it cannot go in any hook except SubagentStop or PreCompact.

**Phase to address:** Phase 1 (architecture decision: hooks vs. orchestrator for discovery).

**Confidence:** HIGH -- directly measured from hook timeout values in `hooks.json` and `settings.json`, current usage estimated from reading each hook's implementation.

---

### Pitfall 29: Agent Prompt Disruption from Injected Research Content

**What goes wrong:**
Adding research content to agent prompts changes agent behavior in unexpected ways. The dsp-agent prompt is carefully tuned over 1,235 lines to produce specific JSON reports, follow real-time safety rules (51 rules with code examples), and use prescribed JUCE DSP patterns. Injecting research content can:

1. **Dilute instruction adherence:** The agent adopts code patterns from the research doc instead of its own prescribed patterns. Example: `reverb-comprehensive-research.md` contains C++ code examples that may use different variable naming, class structure, or API patterns than dsp-agent.md specifies.
2. **Create conflicting instructions:** Research docs may contain recommendations that contradict agent constraints. A research doc might suggest `std::function` for callback patterns, while dsp-agent's real-time safety rules (lines 586-636) explicitly reject ALL `std::function` in processBlock with zero tolerance.
3. **Shift attention from contracts:** The agent's prompt carefully directs it to read ARCHITECTURE.md, parameter-spec.md, and ROADMAP.md first (lines 228-246). Injected research content competes for the same attention budget.

**Why it happens:**
LLMs process all input as a flat token stream. They cannot reliably distinguish "this is your core instruction set" from "this is supplementary reference material." The more tokens of reference material, the more attention budget consumed processing it instead of following instructions. Published research on prompt injection confirms LLMs struggle to maintain instruction boundaries when data is co-located with instructions.

**Warning signs:**
- Agent output format changes (JSON report structure drifts from subagent-report.json)
- Agent uses code patterns from research docs instead of template library patterns
- Real-time safety violations increase (agent follows research code examples that aren't processBlock-safe)
- Agent skips self-validation steps (safety checklist, lines 1136-1150 of dsp-agent.md)
- Agent references research content but misses contract requirements

**How to avoid:**
1. **Inject at END of prompt, after all instructions:** Use clear delimiters:
   ```
   <agent_instructions>
   [existing dsp-agent.md content]
   </agent_instructions>
   <reference_material>
   The following research is for reference. Your instructions above take priority.
   If research content contradicts your real-time safety rules, follow your rules.
   [injected content]
   </reference_material>
   ```
2. **Prefer file paths over inline content:** Instead of injecting full docs, inject: "Relevant: `research/reverb-comprehensive-research.md` (Sections 6-8 for JUCE patterns)". Agent uses Read tool when needed. Keeps prompt small, gives agent agency.
3. **Add explicit priority directive to agent prompts:** "If any research document content contradicts your instructions (especially real-time safety rules), your instructions take precedence unconditionally."
4. **Regression test with known-good plugins:** Inject research for a completed plugin (e.g., O-Bells) and compare output with/without injection. Quality should improve or stay same, never degrade.

**Phase to address:** Phase 2 (injection format) + Phase 4 (regression testing).

**Confidence:** HIGH -- agent prompt sizes measured, research doc sizes measured, instruction-data confusion well documented in LLM research.

---

### Pitfall 30: Accountability Theater (Listing Resources Without Using Them)

**What goes wrong:**
Agents report `resources_consulted: ["reverb-comprehensive-research.md"]` but produce identical output to non-injected runs. The accountability system shows 100% resource consultation but 0% actual knowledge integration. This creates a false sense that resource orchestration is "working" and prevents investigating why output quality hasn't improved.

**Why it happens:**
LLMs are trained to comply with instructions. If told "report which resources you consulted," the agent lists whatever resources were in its context. The citation is mechanically easy (copy filenames from the prompt), but meaningful integration is hard (understand content, apply to implementation, adapt patterns). This is structurally identical to students citing sources they didn't read.

**Warning signs:**
- Agent reports list ALL injected resources every time (100% "consultation" rate is suspicious)
- Removing research docs from injection produces identical code output
- Agent's DSP code doesn't reflect techniques from cited research (e.g., reports consulting reverb doc but implements a basic feedback delay)
- A/B comparison shows no measurable difference between injected and non-injected runs

**How to avoid:**
1. **Verify by output, not self-report:** Post-agent validation checks: "Does the implementation use techniques from the injected research?" For reverb, check: does code reference Dattorro, FDN, or allpass chains documented in the research?
2. **Require specific citations:** Instead of `resources_consulted`, require: `resources_applied: [{"file": "reverb-comprehensive-research.md", "section": "Section 2.4 Dattorro Plate", "application": "Used plate topology with input diffusion = 0.75"}]`. Specific citations are harder to fabricate.
3. **Inject actionable directives, not raw reference:** Instead of the full doc, inject: "Use the Dattorro plate reverb topology (reverb-comprehensive-research.md Section 2.4). Key parameters: input diffusion 1 = 0.75, input diffusion 2 = 0.625, decay diffusion = 0.7." Actionable directives produce verifiable output.
4. **Prove value before adding accountability:** Focus first on whether injection improves output quality (measure via critic scores). Add reporting only after confirming injection actually helps. If it doesn't help, accountability is waste.

**Phase to address:** Phase 3 (accountability), but only after Phase 2 (injection) proves value.

**Confidence:** MEDIUM -- LLM compliance theater is well-documented but difficult to measure precisely without A/B testing in this specific system.

---

### Pitfall 31: Over-Engineering Discovery for a 23-Document Corpus

**What goes wrong:**
The team builds a semantic search engine, embedding pipeline, vector store, or NLP classification system to find relevant documents among 23 files. This takes weeks, requires external dependencies, and is less reliable than a hand-written lookup table.

**Why it happens:**
Semantic search and RAG are the "standard" approach for resource discovery in LLM systems. Tutorials and articles describe embedding-based retrieval as the correct pattern. But those approaches target corpora of thousands to millions of documents. For 23 files in a narrow domain, embeddings add failure modes (similarity thresholds, chunk boundaries, stale embeddings) without solving problems that a 30-line JSON file solves instantly.

**Warning signs:**
- More code written for discovery system than for agents using it
- Discovery requires external dependencies (embedding models, vector databases, sentence-transformers)
- Discovery accuracy lower than a hand-curated lookup table
- System requires "tuning" (similarity thresholds, chunk sizes, overlap parameters) for 23 documents
- New research docs require re-indexing/re-embedding

**How to avoid:**
1. **Start with a static manifest (10 minutes of work):**
   ```json
   {
     "reverb": ["reverb-comprehensive-research.md"],
     "fft": ["fft-processing-best-practices.md", "fft-artifact-prevention.md", "custom-fft-implementations.md"],
     "delay": ["delay-effects-comprehensive-guide.md"],
     "spectral": ["spectral-sequencer-research.md", "spectral-transient-shaper-research.md"],
     "generative": ["generative-audio-algorithms-reference.md", "generative-plugins-research-synthesis.md"],
     "microtonality": ["microtonality-implementation-juce.md", "microtonality-comprehensive-database.md", "microtonality-theory-formats.md", "microtonality-commercial-performance.md"],
     "physical-modeling": ["physical-modeling-commercial-analog-modeling-ml-approaches.md", "physical-modeling-research-agent-3-physical-modelling-optimization.md", "circuit-modeling-fundamentals.md"],
     "modal-synthesis": ["modal-synthesis-bells-academic-research.md", "multi-stage-decay-envelopes-comparison.md"],
     "click-prevention": ["dsp-click-prevention-debugging.md"],
     "webgl": ["webgl-spectrogram-patterns.md"]
   }
   ```
2. **Upgrade to keyword matching only when corpus exceeds 50 docs.**
3. **Upgrade to semantic search only when corpus exceeds 200 docs.**
4. **Document the decision explicitly:** "Static manifest chosen because 23 docs. Reassess at 50 docs." This prevents future contributors from prematurely over-engineering.

**Phase to address:** Phase 1 (explicitly choose simple approach as a design decision).

**Confidence:** HIGH -- corpus is exactly 23 documents (verified). Research confirms simple approaches outperform complex ones on small corpora by eliminating tuning overhead and false positives.

---

### Pitfall 32: Stale Research Documents Injected Without Freshness Signal

**What goes wrong:**
Research docs written months ago contain outdated JUCE API patterns or superseded algorithm recommendations, but the discovery system keeps injecting them because they match topic keywords. Example: `reverb-comprehensive-research.md` (created 2026-01-13) recommends specific JUCE classes. If JUCE 8 deprecates or changes those classes in an update, the research doc still recommends the old approach.

**Why it happens:**
Research documents are written once and referenced indefinitely. There is no freshness signal in the current file structure -- no creation date in structured metadata, no "last verified" timestamp, no expiration policy. The discovery system treats all docs as equally current.

**Warning signs:**
- Agent implements patterns from a research doc that reference a different JUCE version than system-config.json (currently 8.0.9)
- Multiple research docs cover same topic with conflicting recommendations (no way to know which is newer)
- Code review catches API patterns that were valid when doc was written but are now deprecated

**How to avoid:**
1. **Add YAML frontmatter with freshness metadata:**
   ```yaml
   ---
   created: 2026-01-13
   last_verified: 2026-01-13
   juce_version: 8.0.9
   status: current  # current | needs-review | deprecated
   superseded_by: null
   ---
   ```
2. **Discovery checks freshness:** If `last_verified` > 60 days old or `juce_version` differs from system-config.json, add warning: "NOTE: Last verified [date] against JUCE [version]. Current: JUCE [current]. Verify patterns."
3. **Separate stable theory from volatile implementation:** DSP algorithm theory (FDN reverb, overlap-add) rarely changes. JUCE integration patterns change with JUCE releases. Only flag implementation sections for freshness review.
4. **Quarterly review trigger in SessionStart.sh:** Non-blocking warning when any doc's `last_verified` exceeds 90 days.

**Phase to address:** Phase 2 (add frontmatter when building manifest).

**Confidence:** MEDIUM -- staleness risk is currently low (system is weeks old, JUCE 8.0.9 is current) but increases over months. Prevention cost is minimal (YAML frontmatter), so worthwhile as preventive measure.

---

### Pitfall 33: Manifest Maintenance Drift

**What goes wrong:**
Every time a new research document is created (by deep-research skill, research-planning-agent, or manually), someone must update the resource manifest. If forgotten, the new doc is invisible to discovery. Over time, manifest drifts from reality -- some entries reference moved/deleted files, new docs aren't indexed.

**Why it happens:**
The system already creates research docs through deep-research skill and research-planning-agent. These agents write to `research/` but have no awareness of a resource manifest. Manual maintenance creates a burden that competes with the primary goal.

**Warning signs:**
- `ls research/*.md | wc -l` (file count) differs from manifest entry count
- Users bypass discovery and tell agents to "read research/X.md" directly
- Manifest hasn't been updated in weeks despite new research being created
- Discovery returns no results for topics with existing research docs

**How to avoid:**
1. **Auto-generate manifest from file metadata:** If docs have YAML frontmatter with topic tags, scan `research/*.md` and extract frontmatter. Run as part of SessionStart.sh (fast -- under 1s for 23 files):
   ```bash
   python3 .claude/scripts/build-resource-manifest.py --dir research/ --output .claude/cache/resource-manifest.json
   ```
2. **Default tags from filename:** If frontmatter missing, derive tags from filename: `reverb-comprehensive-research.md` -> `["reverb"]`. Ensures basic discoverability without explicit tagging.
3. **Make generation idempotent:** Running twice produces identical output. No corruption risk.
4. **Update deep-research skill to include frontmatter:** When creating new research docs, include the YAML template. Zero-cost maintenance for new docs.
5. **Validation in SessionStart.sh:** After generating manifest, verify every `research/*.md` file has an entry and every manifest entry points to existing file. Log warnings for mismatches.

**Phase to address:** Phase 2 (build auto-generation) + Phase 3 (update deep-research skill to emit frontmatter).

**Confidence:** HIGH -- maintenance drift is a predictable consequence of manual processes. Auto-generation from metadata is proven and cheap for 23 files.

---

## v1.2 Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Inject full research docs into prompts | No extraction logic needed | Context exhaustion, attention dilution, higher token costs | Never -- even for testing, use extracts or paths |
| Hard-code resource mappings in agent .md files | Quick per-agent | Unmaintainable as corpus grows, agents updated independently | Only as <1 week prototype |
| Skip schema versioning for resource fields | Faster deployment | Cascading failures when fields change again, no rollback path | Never -- CHANGELOG.md process exists |
| Put discovery in PostToolUse.sh | Runs on every Write/Edit | 2s timeout insufficient, slows every file operation | Never -- PostToolUse is for validation only |
| Use `additionalProperties: true` to avoid schema issues | No validation failures | Loses typo detection, data integrity | Only for explicitly extensible metadata objects |
| Copy-paste research into agent prompts manually | Works immediately | Inconsistent, no freshness tracking, no automation | Only during initial proof-of-concept |
| Build semantic search for 23 docs | "Proper" architecture | Weeks of work, more failure modes than static lookup | Never for current corpus size |

## v1.2 Integration Gotchas

| Integration Point | Common Mistake | Correct Approach |
|-------------------|----------------|------------------|
| PostToolUse.sh (2s timeout) | Adding discovery logic | Keep as validation-only; discovery goes in orchestrator |
| Agent prompt construction | Injecting research before instructions | Inject after instructions with `<reference_material>` delimiter and priority directive |
| SubagentStop.sh (10s timeout) | Validating resource usage here | Validate in agent's own self-check (prompt-based), not external hook |
| subagent-report.json schema | Adding required resource fields | Add optional fields with defaults; MINOR version bump |
| PreCompact.sh | Not preserving resource state | Add manifest path to PreCompact preservation list |
| Contract checksums | Resource injection triggering checksum failures | Resource injection is separate from contracts; checksums cover contract files only |
| UserPromptSubmit.sh | Making discovery part of `/continue` flow | Discovery runs at agent invocation time in orchestrator skill |
| deep-research skill | Creating docs without manifest metadata | Update skill to emit YAML frontmatter in new docs |

## v1.2 Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Full-text search over all 23 research docs | Slow, false positives everywhere | Use metadata tags or static manifest | Immediate for keyword ambiguity; O(n) at 50+ docs |
| Injecting all matched docs into agent | Context overflow, quality drop | Cap at 2-3 docs max, extract sections only | Immediate -- even 2 full docs (~22K tokens) overwhelm |
| Synchronous discovery in hooks | Timeout failures | Cache results or run in orchestrator | Immediate for PostToolUse; risky for UserPromptSubmit |
| Re-discovering per file write | Redundant work | Discover once per agent invocation, cache for session | Immediate -- discovery runs once, not per-operation |
| Embedding all 23 docs for vector search | Slow startup, stale embeddings | Static manifest (no embeddings needed for 23 docs) | Immediate -- over-engineering from day one |

## v1.2 "Looks Done But Isn't" Checklist

- [ ] **Discovery returns right docs:** Test with 5+ plugin types (reverb, delay, spectral, generative, microtonality). Check each result is relevant, not just "some result."
- [ ] **Agents use resources meaningfully:** Compare output with and without injection for same plugin. If identical, injection isn't working regardless of what agents report.
- [ ] **ALL 6 subagent-report.json consumers updated:** Schema + foundation-shell-agent + dsp-agent + gui-agent + ui-design-agent + ui-finalization-agent + research-planning-agent. Partial updates cause silent failures.
- [ ] **Hooks don't timeout under load:** Test with warm AND cold disk cache. Test with all 23 docs in corpus. Time each hook independently.
- [ ] **Manifest covers all docs:** `ls research/*.md | wc -l` must equal manifest entry count.
- [ ] **Freshness metadata present:** Every doc has YAML frontmatter with `created`, `last_verified`, `juce_version`.
- [ ] **Backward compatibility preserved:** Run plugin-testing skill on a known-good plugin with new resource code. Must pass identically.
- [ ] **Context budget verified:** Measure tokens: agent prompt + injected resources + contracts. Must be < 40K tokens total.
- [ ] **Graceful degradation works:** Rename manifest file. Verify agents still run and produce valid output. If they crash, you built a hard dependency.
- [ ] **No PostToolUse.sh timeout:** PostToolUse must complete in < 2s with resource code active. Benchmark: `time .claude/hooks/PostToolUse.sh < test-input.json`.

## v1.2 Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Context window exhaustion | LOW | Reduce injection to paths only, revert to no injection temporarily |
| False relevance / wrong docs | LOW | Update manifest tags, switch to curated lookup, add exclusion rules |
| Broken schemas | MEDIUM | Revert schema, update all 6 agents atomically, re-deploy |
| Hook timeout failures | LOW | Move discovery from hook to orchestrator skill |
| Agent prompt disruption | MEDIUM | A/B test, adjust injection format, add priority directive, revert if needed |
| Accountability theater | LOW | Remove reporting, focus on output-based verification instead |
| Over-engineered discovery | HIGH | Tear down complex system, replace with static manifest (sunk cost lost) |
| Stale docs injected | LOW | Add freshness warning, update `last_verified` dates |
| Manifest drift | LOW | Run auto-generation script, fix deep-research skill |
| Discovery SPOF | MEDIUM | Add graceful degradation, ensure agents work without discovery |

## v1.2 Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| #24 Context exhaustion | Phase 1 (design injection budget) | Measure tokens per agent invocation; must be < 40K |
| #25 False relevance | Phase 1 (design tags) + Phase 2 (build manifest) | Test with 5+ plugin types, verify precision |
| #26 Breaking schemas | Phase 1 (plan schema evolution) | jsonschema validation with old AND new sample data |
| #27 Discovery SPOF | Phase 2 (implement graceful degradation) | Break discovery intentionally, verify agents still work |
| #28 Hook timeouts | Phase 1 (choose hooks vs. orchestrator) | Benchmark discovery; must complete < 1s |
| #29 Prompt disruption | Phase 2 (injection format) + Phase 4 (regression test) | A/B test on known-good plugin |
| #30 Accountability theater | Phase 3 (only after injection proven useful) | Compare output with/without injection |
| #31 Over-engineering | Phase 1 (explicitly choose static manifest) | Decision documented: "static because 23 docs" |
| #32 Stale resources | Phase 2 (add frontmatter to all docs) | All 23 docs have YAML frontmatter with dates |
| #33 Manifest drift | Phase 2 (auto-gen script) + Phase 3 (update deep-research) | Add new test doc; verify it appears without manual steps |

## v1.2 Pre-Implementation Checklist

Before starting v1.2 resource orchestration:

- [ ] Inventory all 23 research docs with sizes (DONE -- see measurements above)
- [ ] Measure all agent prompt sizes in tokens (DONE -- ranges 126-1,301 lines)
- [ ] Document all hook timeout budgets (DONE -- see table above)
- [ ] Decide: hooks vs. orchestrator-level discovery (Phase 1 decision)
- [ ] Decide: schema extension vs. parallel schema (Phase 1 decision)
- [ ] Decide: static manifest vs. dynamic discovery (Phase 1 decision -- recommend static)
- [ ] Set token budget ceiling per agent (recommend: 4,000 tokens max injected research)
- [ ] Identify test plugins for regression testing (recommend: O-Bells, O-Freeze)
- [ ] Review `additionalProperties: false` implications for ALL schemas being modified

---

## v1.2 Sources

**Context Window Management (HIGH confidence):**
- [Chroma Research: Context Rot](https://research.trychroma.com/context-rot) -- 18 LLMs measured, performance degrades with input length
- [NoLiMa Benchmark / Towards Data Science](https://towardsdatascience.com/your-1m-context-window-llm-is-less-powerful-than-you-think/) -- 11/12 models below 50% at 32K tokens
- [Claude API: Context Windows](https://platform.claude.com/docs/en/build-with-claude/context-windows) -- 200K token window documentation
- [Claude Code Best Practices](https://code.claude.com/docs/en/best-practices) -- /compact at 70%, subagent isolation

**Resource Discovery & RAG (MEDIUM-HIGH confidence):**
- [InfoQ: Reducing RAG False Positives](https://www.infoq.com/articles/reducing-false-positives-retrieval-augmented-generation/) -- Banking case study, 99% to 3.8%
- [Label Studio: Seven RAG Pitfalls](https://labelstud.io/blog/seven-ways-your-rag-system-could-be-failing-and-how-to-fix-them/) -- Chunking, ranking failures
- [Elastic: Context Engineering for AI Agents](https://www.elastic.co/search-labs/blog/context-engineering-relevance-ai-agents-elasticsearch) -- Hybrid search, context rot defense
- [Towards Data Science: Over-Engineered Retrieval](https://towardsdatascience.com/how-to-build-an-overengineered-retrieval-system/) -- Complexity vs. simplicity tradeoffs

**Schema Evolution (HIGH confidence):**
- [Creek Service: Evolving JSON Schemas Part I](https://www.creekservice.org/articles/2024/01/08/json-schema-evolution-part-1.html) -- Closed/open model evolution rules
- [Creek Service: Evolving JSON Schemas Part II](https://www.creekservice.org/articles/2024/01/09/json-schema-evolution-part-2.html) -- Partially open models

**Agent Prompt Engineering (MEDIUM confidence):**
- [Simon Willison: Prompt Injection Design Patterns](https://simonwillison.net/2025/Jun/13/prompt-injection-design-patterns/) -- Context minimization, instruction boundaries
- [LangChain: Context Engineering in Agents](https://docs.langchain.com/oss/python/langchain/context-engineering) -- Middleware injection patterns

**Claude Code Specific (HIGH confidence):**
- [Factory.ai: Context Window Problem](https://factory.ai/news/context-window-problem) -- Agent token accumulation
- [Claude Code Dynamic Context Injection](https://www.365iwebdesign.co.uk/news/2026/01/23/claude-code-dynamic-context-injection/) -- DCI patterns
- [Medium: Claude Code MCP Context Bloat Reduction](https://medium.com/@joe.njenga/claude-code-just-cut-mcp-context-bloat-by-46-9-51k-tokens-down-to-8-5k-with-new-tool-search-ddf9e905f734) -- Tool search reducing 51K to 8.5K tokens

**Direct Codebase Inspection (HIGH confidence -- primary source):**
- `.claude/agents/*.md` -- all 11 agent prompts measured
- `.claude/hooks/*.sh` + `hooks.json` + `settings.json` -- all 6 hooks with timeouts
- `.claude/schemas/subagent-report.json` -- confirmed `additionalProperties: false`
- `research/*.md` -- all 23 documents sized (lines and KB)

---

*v1.2 Addendum Researched: 2026-02-04*
*Focus: Pitfalls specific to adding resource orchestration to an existing 13-agent system with JSON Schema contracts and hook-based validation*

---

---

# Addendum: v1.3 System Modernization Pitfalls (Opus 4.6 + GSD Alignment)

**Domain:** Modernizing an existing AI agent orchestration system (Plugin Freedom System) to leverage Opus 4.6 capabilities and align with GSD 1.18.0
**Researched:** 2026-02-08
**Confidence:** HIGH (based on direct codebase analysis of 13 agents, 40+ commands, 12 validators, 6 scripts, 3 hooks; verified against official Claude Code docs and Opus 4.6 release notes)

This addendum covers pitfalls specific to the v1.3 milestone: modernizing the Plugin Freedom System to leverage Opus 4.6 (agent teams, 1M context beta, improved reasoning) and align with GSD 1.18.0 features, eliminating custom code where the framework now provides native support. These pitfalls are about what goes wrong when you MODERNIZE an existing system with extensive custom code layered on top of a framework that has evolved independently.

**System measurements at time of research:**
- 13 agents in `.claude/agents/` (11 .md files, 2 implicit critics)
- 40+ commands in `.claude/commands/`
- 12 Python validators in `.claude/hooks/validators/`
- 6 Python scripts in `.claude/scripts/`
- 3 shell hooks (SessionStart, PostToolUse, PreCompact)
- JSON Schema contracts with `additionalProperties: false` (subagent-report.json, validator-report.json)
- GSD 1.18.0 installed globally at `~/.claude/get-shit-done/`
- 35+ completed plugins depending on working workflows
- 62+ requirements satisfied across 3 milestones (38 plans, 13 phases)
- Resource discovery system (v1.2) operational with 27 research docs, 4K token budget, 63ms discovery

---

## Critical Pitfalls (v1.3)

### Pitfall 34: Removing Custom Code That Has PFS-Specific Behavior Beyond Framework Duplicates

**What goes wrong:**
During deduplication, custom PFS code is removed because it appears to duplicate GSD framework features. But the custom code contains domain-specific behavior that the framework does not replicate. The system silently degrades: plugins build but agents produce lower-quality output, skip validation steps, or lose context they previously received.

**Why it happens:**
The PFS has layered custom code on top of GSD over three milestones (62+ requirements). Much of this code started as workarounds for missing GSD features but accumulated JUCE-specific and plugin-workflow-specific logic over time. Key examples:

- **PostToolUse hook** validates real-time safety violations specific to JUCE `processBlock` (heap allocation, mutex locks, file I/O, console output, ScopedNoDenormals) -- GSD has no concept of "real-time safety"
- **PostToolUse hook** enforces contract immutability during Stages 1-4 -- GSD phases have no "immutable contracts" concept
- **PreCompact hook** preserves plugin contracts (creative-brief, parameter-spec, architecture, plan) -- GSD's compaction preserves STATE.md but not domain-specific contract files
- **Resource discovery system** (`discover-resources.py`, `inject-context.py`) injects domain-specific research into agents within a 4K token budget -- GSD's research phase serves a different purpose (domain exploration, not per-agent context injection)
- **Handoff protocol** enforces `/clear` + next command patterns specific to the 5-stage serial workflow -- GSD's phase transitions offer `/clear` as a footnote, not a mandatory two-step sequence
- **Quality gates** (`run-gate.sh`) run stage-specific validators (schema, build, pluginval, DSP critic, UI critic) -- GSD verification is goal-backward checking, not stage-specific validation chains

A naive diff of "custom code vs framework" will flag these as duplicates when they are domain extensions.

**Warning signs:**
- Agent output quality drops after removing code (less specific, more generic DSP/UI output)
- Validation gates pass when they should fail (real-time safety violations not caught)
- Handoff messages lose the `/clear` instruction or full plugin name
- Resource discovery stops injecting research context (check `resources_consulted` in agent reports -- should not be empty)
- PostToolUse hook no longer catches `new`/`malloc` in processBlock code
- Contract modifications silently succeed during implementation stages

**How to avoid:**
1. Before removing ANY custom code, create a behavior specification documenting exactly what it does, which files it applies to, and what it blocks or enables
2. Classify each piece of custom code into one of three types:
   - **Pure duplicate** (safe to remove): Custom code that reimplements a GSD feature identically with no PFS-specific behavior. Example: a custom progress reporting function that GSD now handles
   - **Extension** (keep, possibly refactor to use framework hooks): Custom code that extends GSD with domain-specific logic. Example: PostToolUse real-time safety validation
   - **Workaround** (investigate): Custom code that works around a GSD limitation that may now be resolved. Example: custom state recovery logic that GSD checkpoints may now handle
3. Test each removal individually by running a real plugin through the full pipeline: `/implement --express` on a known-good plugin (e.g., O-SimpleReverb) after each removal
4. Create a "before/after" comparison for each removal showing the behavior preserved or lost

**Phase to address:**
Phase 1 (System Audit) -- must classify ALL custom code before any removal happens in later phases. The classification deliverable blocks all subsequent phases.

---

### Pitfall 35: Breaking Agent Contracts During Opus 4.6 Frontmatter Migration

**What goes wrong:**
Updating agent definitions to use Opus 4.6 frontmatter features (new fields: `skills`, `memory`, `permissionMode`, `maxTurns`, `hooks`, `mcpServers`) while simultaneously changing report schemas creates cascading contract violations. The `/implement` workflow breaks mid-plugin because an agent produces output the orchestrator cannot parse, or the orchestrator invokes agents with unrecognized configuration.

**Why it happens:**
The PFS uses tight coupling between agent definitions, schemas, and orchestrator code:

1. **Agent enum in schema:** `subagent-report.json` has `"agent": {"enum": ["research-planning-agent", "foundation-shell-agent", "dsp-agent", "gui-agent", "ui-design-agent", "ui-finalization-agent"]}` -- adding or renaming any agent breaks this enum
2. **`additionalProperties: false`:** Adding even one new field to agent output (like a new `memory_updated` field from persistent memory) breaks validation
3. **Model routing:** Agents currently specify `model: sonnet` or `model: opus` in frontmatter. The PFS config has `model_profile: "quality"` which GSD uses for its own agents. PFS agent frontmatter and GSD model profiles can conflict
4. **Orchestrator expectations:** The `plugin-execute.md` command and `implement.md` command reference specific agent names and expect specific report structures. Changes cascade through 8+ skill files that reference agents
5. **Validator dependencies:** 12 Python validators parse agent output in specific formats. Schema changes require validator updates

**Warning signs:**
- JSON parse errors in orchestrator output after agent changes
- Agent reports rejected by `additionalProperties: false` constraint
- Orchestrator showing "unknown agent" when agents are renamed
- Model selection producing unexpected results (Sonnet agent running on Opus or vice versa)
- Validators crashing on new report structure

**How to avoid:**
1. **Schema changes must be additive only:** Add new optional fields with defaults, never remove or rename existing ones. Use MINOR version bumps per existing CHANGELOG.md process
2. **Before changing any agent definition, map all references:**
   ```bash
   grep -r "foundation-shell-agent" .claude/ --include="*.md" --include="*.json" --include="*.py" --include="*.sh" | wc -l
   ```
3. **Atomic updates:** When modifying a schema, update ALL agent definitions + ALL validators + ALL orchestrator references in the same commit
4. **Keep old frontmatter fields alongside new ones during transition:**
   ```yaml
   model: sonnet          # existing (keep)
   permissionMode: default # new Opus 4.6 field (add)
   ```
5. **Run the full `/implement --express` pipeline on a canary plugin after every schema or agent change**

**Phase to address:**
Phase 2 (Agent Modernization) -- schema compatibility must be the FIRST thing verified before any agent definition changes. Create a schema migration plan as the phase's first deliverable.

---

### Pitfall 36: Agent Teams Applied to the Serial Stage Pipeline

**What goes wrong:**
The Opus 4.6 agent teams feature is adopted for the PFS stage pipeline (Foundation -> DSP -> GUI -> Polish), but this pipeline is inherently serial with strict file-level dependencies. Agent teams add coordination overhead, increase token costs by 5-10x, and introduce file conflict risks with zero parallelism benefit. Worse, two teammates writing to the same file means "last write wins" per the official docs.

**Why it happens:**
Agent teams are the headline Opus 4.6 feature. There is natural pressure to adopt them because they are new and prestigious, not because the workflow benefits. The PFS stage pipeline has these properties that make it unsuitable for agent teams:

- **Strict ordering:** Each stage modifies the same files (PluginProcessor.cpp, PluginEditor.cpp, CMakeLists.txt)
- **Contract dependencies:** DSP agent reads Foundation output; GUI agent reads DSP output
- **Single-file ownership:** processBlock() is written by DSP agent and later modified by Polish agent
- **Validation gates:** Each gate requires the previous stage's binary output to exist and pass pluginval
- **Official limitation:** "No built-in file locking. Two teammates writing to the same file = last write wins" (Claude Code docs)
- **Official limitation:** "No session resumption with in-process teammates" -- cannot recover mid-plugin
- **Official limitation:** "No nested teams" -- teammates cannot spawn their own subagents, which PFS agents currently rely on for Context7 queries

**How to avoid:**
1. Keep the existing serial stage pipeline unchanged -- it works well and has been battle-tested across 35+ plugins
2. Use agent teams ONLY for genuinely parallel, independent work:
   - **Research phase:** Multiple researchers investigating different aspects (GSD already does this with 4 parallel researchers via subagents)
   - **Cross-plugin batch operations:** Auditing, upgrading, or fixing multiple plugins simultaneously (different directories, no file conflicts)
   - **Competing hypothesis debugging:** When a build fails, spawn multiple investigators exploring different root causes
3. Use subagents (Task tool) for the existing stage delegation pattern -- this is what the PFS already does successfully and is the recommended approach per official docs for "focused tasks where only the result matters"
4. Document explicitly in the v1.3 plan: "Agent teams are for parallel independent work; the stage pipeline stays serial with subagents"
5. Create a decision matrix in the audit phase mapping each PFS workflow to the correct primitive

**Warning signs:**
- Proposal to run Foundation and DSP agents simultaneously
- Agent team being used with fewer than 3 independent workers doing independent tasks
- File conflicts appearing in git (merge markers in source files)
- Token costs increasing 5-10x without proportional productivity gain
- Agents failing because they cannot spawn subagents from within a teammate context

**Phase to address:**
Phase 1 (System Audit) -- create a "parallel suitability matrix" classifying every PFS workflow as serial-only, potentially-parallel, or already-parallel. All subsequent phases reference this matrix.

---

### Pitfall 37: Confusing Subagents, Agent Teams, and Skills (Terminology Collision)

**What goes wrong:**
The modernization plan conflates three distinct Claude Code primitives with different tradeoffs. The PFS already has its own abstraction layer with overlapping names. The result: Skills used where subagents are needed (no isolation, context bleed into main conversation), agent teams used where subagents suffice (massive token overhead), or subagents used where simple skill injection would suffice (unnecessary context switching).

**Why it happens:**
The PFS naming and Claude Code naming collide:

| PFS Term | PFS Location | Claude Code Equivalent | Key Difference |
|----------|-------------|----------------------|----------------|
| "Agent" | `.claude/agents/*.md` | Subagent | PFS agents ARE Claude Code subagents |
| "Command" | `.claude/commands/*.md` | Command | Same concept |
| "Skill" (frontmatter `skill:` in commands) | Command routing | Partial overlap with Claude Code Skills | PFS "skill" routes to orchestration logic; Claude Code "skill" is a prompt + config that can run in forked context |
| (no equivalent) | -- | Agent Team | Multi-session parallel coordination; experimental |
| (no equivalent) | -- | Skill (`context: fork`) | Subagent constructor from skill file |
| (no equivalent) | -- | Persistent Memory (`memory:` frontmatter) | Cross-session agent memory |

Official docs explicitly note: "Subagents, Commands and Skills Are Converging" -- the boundaries are blurring. This means adopting new primitives requires careful mapping to avoid breaking working abstractions.

Additionally, Claude Code subagents have important constraints the PFS depends on:
- Subagents cannot spawn other subagents (but PFS agents can use Task tool within GSD orchestration)
- Skills with `context: fork` have a known issue where the fork may not work when invoked via Skill tool (GitHub issue #17283)

**Warning signs:**
- Plans describing work as "agent team" when it is actually subagent orchestration
- Skills being loaded into main context adding 50K+ tokens without isolation
- Agent teams spawned for 2-agent sequential work (overhead exceeds benefit)
- PFS commands accidentally triggering Claude Code's built-in skill matching instead of PFS routing
- Confusion about whether `memory:` field goes on a PFS agent or a GSD agent

**How to avoid:**
1. Create a terminology mapping document as the FIRST deliverable of Phase 1:

   | When you need... | Use this | Not this |
   |-----------------|---------|----------|
   | Isolated execution with fresh context, JSON report back | **Subagent** (current PFS pattern) | Agent team (overkill) |
   | Parallel independent workers that communicate | **Agent team** (new, experimental) | Multiple subagents (can't communicate) |
   | Reusable prompt injection in current context | **Skill** (new) | Subagent (unnecessary isolation) |
   | Cross-session learning | **Persistent memory** (evaluate carefully) | Ad-hoc file writing |
   | User entry point | **Command** (existing pattern) | Skill (wrong abstraction) |

2. Map every existing PFS workflow to its correct primitive before changing anything
3. Do NOT adopt agent teams or persistent memory until the terminology mapping is complete and validated against real workflows

**Phase to address:**
Phase 1 (System Audit) -- terminology mapping is a prerequisite for all agent modernization work.

---

### Pitfall 38: Context Window Overconfidence with 1M Token Beta

**What goes wrong:**
Opus 4.6 offers a 1M token context window (beta), creating a false sense that context management is no longer important. Agent definitions are made larger, skills load more content, research injection budgets are relaxed, and token budgets are treated as obsolete constraints. The result: compaction events occur more frequently during critical implementation steps, agents lose their instructions mid-task, and plugin output quality drops.

**Why it happens:**
The PFS has carefully managed context budgets based on hard-learned lessons:
- Research injection: 4,000 token budget cap (max observed: 3,478 tokens)
- Validation reports: 500-token budget
- Agent definitions: Already large (foundation-shell-agent is 1,020 lines, dsp-agent is 1,260 lines)
- PreCompact hook preserves contracts specifically because compaction loses them

With 1M tokens available, the natural instinct is to relax these limits. But:
- **1M context is in beta** and may not be reliable or available for all deployments
- **Subagents get fresh 200K context windows** regardless of main context size -- the 1M limit applies to the orchestrator, not to subagents
- **MCP tools reduce available context:** "Your context window can shrink from 200K to 70K with too many tools" (official docs)
- **Performance degrades with length:** Published research (NoLiMa, Chroma context rot) shows accuracy drops non-linearly with context size, even in models claiming large windows
- **Compaction still happens:** Even with 1M tokens, auto-compaction triggers at ~95% capacity. Larger context just means more tokens to lose during compaction
- **Token costs scale linearly:** 1M tokens of context means higher per-request costs

**Warning signs:**
- Agents requesting information they should already have (contracts, parameter lists) -- indicates compaction lost them
- Compaction events occurring during critical implementation steps (mid-processBlock implementation)
- Agent output becoming generic rather than specific to the plugin being built
- Token costs per plugin build increasing >50% without quality improvement
- PreCompact hook output growing beyond 5K tokens (too much contract state to preserve reliably)

**How to avoid:**
1. **Keep existing token budgets** (4K research injection, 500-token validation reports) -- they work and are proven
2. **Do NOT increase agent definition size.** If adding Opus 4.6 features (skills, memory), reduce other content proportionally. The constraint drives quality
3. **Measure before relaxing:** Run `/implement --express` on a canary plugin before and after any context changes. Record compaction event count and locations. Compare
4. **Keep PreCompact hook** -- even with 1M context, contract preservation during compaction remains critical
5. **Treat 1M context as a safety margin, not a budget increase:** The extra capacity means fewer emergency compactions, not permission to fill the window

**Phase to address:**
Phase 3 (Context & Intelligence) -- dedicated phase for context management optimization, but only after agent changes are stable from Phase 2.

---

### Pitfall 39: GSD Framework Updates Breaking PFS Custom Integration Points

**What goes wrong:**
GSD 1.18.0 (or a future update) changes internal APIs, file paths, or tool interfaces that PFS custom code depends on. Since GSD is installed globally at `~/.claude/get-shit-done/` and can be updated independently of the PFS project, an update can silently break PFS without any PFS code changes.

**Why it happens:**
The PFS integrates with GSD at multiple fragile points:

1. **Direct tool calls with expected JSON output:**
   - Commands reference `node ~/.claude/get-shit-done/bin/gsd-tools.js init execute-phase "${PHASE_ARG}"` and parse specific JSON fields (`executor_model`, `phase_found`, `phase_dir`, etc.)
   - If `gsd-tools.js` changes its output schema, PFS commands break

2. **Absolute path references to GSD templates:**
   - GSD workflows reference templates with absolute paths: `/Users/taylorbrook/.claude/get-shit-done/templates/summary.md`
   - These paths are embedded in GSD workflow files, not PFS files, but PFS depends on their existence

3. **Shared config with undocumented contracts:**
   - `.planning/config.json` is read by both GSD (`model_profile`, `workflow.research`, `parallelization`) and PFS (custom extensions)
   - No schema defines which fields belong to GSD vs PFS

4. **GSD workflow entry points:**
   - PFS commands like `/implement` wrap GSD phases but add custom pre/post processing
   - GSD phase lifecycle changes (new hooks, different step ordering) can break PFS wrappers

5. **Model profile resolution:**
   - GSD's `references/model-profiles.md` defines model routing for GSD agents (gsd-planner, gsd-executor, etc.)
   - PFS agents define their own model in frontmatter
   - If GSD changes how model resolution works, PFS agent model selection may break

**Warning signs:**
- `gsd-tools.js` commands returning unexpected JSON structure or new required fields
- Template file-not-found errors referencing `~/.claude/get-shit-done/templates/`
- `.planning/config.json` gaining new required fields that PFS does not provide
- GSD workflows calling hooks in a different order than PFS expects
- Model selection producing unexpected results after GSD update

**How to avoid:**
1. **Create an integration contract document** listing every GSD interface the PFS depends on (commands, expected JSON schemas, template paths, config fields, hook trigger order)
2. **Before updating GSD**, diff the old and new versions: `diff -r ~/.claude/get-shit-done/ ~/get-shit-done-backup/`
3. **Add integration smoke test** that verifies GSD interfaces:
   ```bash
   # Verify gsd-tools.js returns expected fields
   node ~/.claude/get-shit-done/bin/gsd-tools.js init new-milestone 2>&1 | python3 -c "import sys,json; d=json.load(sys.stdin); assert 'roadmapper_model' in d"
   ```
4. **Pin GSD version** in documentation and verify the installed version at session start
5. **Adopt new GSD features incrementally** (one feature at a time) with a canary plugin build after each adoption

**Phase to address:**
Phase 1 (System Audit) -- catalog all GSD integration points as a deliverable. Phase 4 (GSD Feature Adoption) -- test each adoption against the integration contract.

---

### Pitfall 40: Regression in Plugin Build Quality During Modernization Period

**What goes wrong:**
During modernization, agent definitions are in a half-updated state. Some agents have new Opus 4.6 features, others use old patterns. A user runs `/implement` on a real plugin during this transition window. The workflow produces a broken plugin because report formats are mismatched, model routing is wrong, or new features interfere with existing prompt behavior. 35+ completed plugins depend on this system.

**Why it happens:**
The PFS is a live system. Users continue building plugins while the system is modernized. Unlike a greenfield project where breaking things until launch is acceptable, the PFS must maintain backward compatibility throughout the transition.

Specific risk vectors:
- Agent A updated to Opus 4.6 patterns, Agent B still using old patterns -- orchestrator receives incompatible reports
- New `permissionMode` setting on an agent changes its tool access, blocking operations that previously worked
- Persistent memory from one plugin leaks into another plugin's workflow
- Skills loaded into an agent change its prompt behavior (skills content injected into system prompt, not just "available")
- GSD model profile resolution conflicts with PFS agent frontmatter `model:` field

Amplifying factors:
- No automated end-to-end test suite for the plugin workflow
- Manual testing requires running a full plugin build (20-30 minutes minimum)
- Agent changes are hard to roll back -- they are markdown files, but their behavioral effects cascade through 8+ orchestrator skills

**Warning signs:**
- Users reporting "something changed" in plugin output quality
- Build failures on plugins that previously built successfully
- Agent model selection not matching expectations (wrong model running)
- Missing handoff messages or quality gate bypasses
- Resource accountability field disappearing from agent reports

**How to avoid:**
1. **Feature-flag approach in config:**
   ```json
   {
     "modernization": {
       "use_opus_46_agents": false,
       "use_new_schemas": false,
       "use_agent_memory": false
     }
   }
   ```
2. **Canary plugin:** Designate a simple effect plugin (low complexity, fast build, known-good output) as the modernization canary. Run `/implement --express` after every change. Compare output against baseline
3. **One agent at a time:** Never modify more than one agent definition per commit. Test the full pipeline after each change
4. **Backup agent definitions:**
   ```
   .claude/agents/dsp-agent.md          (active)
   .claude/agents/dsp-agent.v12.md      (backup -- last known-good)
   ```
5. **Gate every change:** No modernization change ships without the canary plugin building successfully end-to-end

**Phase to address:**
All phases -- this is a cross-cutting concern. Canary testing infrastructure should be established in Phase 1 and used throughout.

---

## Moderate Pitfalls (v1.3)

### Pitfall 41: Adopting Persistent Memory Without Data Hygiene Strategy

**What goes wrong:**
The `memory` frontmatter field is added to PFS agents, giving them persistent state across sessions. Agents accumulate stale patterns, outdated plugin information, and conflicting recommendations. Over time, memory becomes a liability: agents recommend deprecated JUCE patterns, reference plugins that have been refactored, or apply conventions from one plugin type to another where they do not apply.

**Why it happens:**
Persistent memory is powerful but requires active curation. The official docs note agents should "curate MEMORY.md if it exceeds 200 lines" but this is passive -- it relies on the agent to decide what to keep. Without a hygiene strategy:
- DSP patterns from early plugins accumulate and override better patterns discovered later
- Plugin-specific quirks get generalized as "best practices"
- Memory grows without bounds, consuming context window space (first 200 lines injected at startup)
- No mechanism to audit or reset agent memory programmatically
- The PFS already has a research discovery system with 90-day staleness detection -- agent memory adds a SECOND knowledge system with no staleness detection

**Warning signs:**
- Agent memory files exceeding 200 lines (triggers automatic curation, which may discard important information)
- Agents recommending deprecated JUCE patterns (version mismatches between memory and current JUCE 8.0.9)
- Agents applying one plugin's conventions to a different plugin type (reverb patterns applied to a synthesizer)
- Memory directories growing unbounded in `.claude/agent-memory/`
- Conflicting information between agent memory and research discovery injection

**How to avoid:**
1. **Evaluate value before adopting:** The resource discovery system (v1.2) already provides domain knowledge injection with structured metadata, staleness detection, and token budgets. Persistent memory adds a second, unstructured knowledge channel. Verify memory provides value ABOVE what discovery already delivers before adding it
2. **If adopting, use `project` scope** (not `user`) so memory is PFS-specific and version-controlled in `.claude/agent-memory/`
3. **Create memory templates** for each agent type defining what to remember and what to discard
4. **Add memory auditing to milestone completion:** Review and prune agent memory before starting a new milestone
5. **Consider NOT using memory initially** -- the resource discovery system may be sufficient. Memory adds complexity without clear incremental benefit in a system that already has structured knowledge management

**Phase to address:**
Phase 3 (Context & Intelligence) -- evaluate whether memory adds value on top of existing resource discovery before adopting. Do not adopt in Phase 2 (Agent Modernization) alongside other changes.

---

### Pitfall 42: State File Format Drift Between GSD Versions

**What goes wrong:**
The PFS stores critical state in `.planning/STATE.md`, `.planning/config.json`, `.planning/ROADMAP.md`, and per-plugin `.planning/STATUS.md` files. GSD reads and writes these files through `gsd-tools.js`. A GSD update changes expected file formats (adds required fields, changes section structure, restructures YAML frontmatter). Existing state files become unparsable, and the system either errors out or silently ignores 3 milestones of accumulated state.

**Why it happens:**
The PFS has accumulated significant state across 3 milestones:
- STATE.md tracks 38 completed plans, 62+ requirements, quick task history, session continuity
- ROADMAP.md has phase records from phases 1-13 with completion dates
- config.json has GSD standard fields (`model_profile`, `parallelization`) plus PFS extensions
- Per-plugin STATUS.md files track stage completion, contract checksums, build artifact paths
- MILESTONES.md records 3 shipped milestones with git ranges, stats, and accomplishments

GSD's `gsd-tools.js init` expects specific state file formats. If GSD 1.18.0+ adds a `milestone_version` field to phase entries or changes how `config.json` model profiles work, existing state may break.

The PFS also extends state files with custom fields that GSD does not know about:
- config.json: `depth`, `commit_docs` (may or may not be GSD fields)
- STATUS.md: YAML frontmatter with `contract_checksums`, `orchestration_mode`, `phased_implementation` (PFS-specific)

**Warning signs:**
- `gsd-tools.js` commands failing with JSON parse errors or "missing field" errors
- STATE.md losing accumulated context (decisions, quick tasks, blockers) after a GSD operation
- ROADMAP.md phase entries not displaying correctly (phase numbering reset, completion dates lost)
- config.json losing PFS-specific settings after GSD writes to it

**How to avoid:**
1. **Snapshot all state files before modernization:**
   ```bash
   cp -r .planning/ .planning-v12-backup/
   ```
2. **Test GSD state file parsing before making changes:**
   ```bash
   node ~/.claude/get-shit-done/bin/gsd-tools.js init new-milestone 2>&1 | python3 -c "import sys,json; print(json.load(sys.stdin))"
   ```
3. **Write a migration script** if format changes are needed (read v1.2 format, write v1.3 format)
4. **Add state file format validation** to SessionStart hook as a non-blocking check
5. **Never modify state files manually** during modernization -- always use migration tooling

**Phase to address:**
Phase 1 (System Audit) -- validate state file compatibility as part of the GSD integration contract.

---

### Pitfall 43: Over-Engineering the Modernization

**What goes wrong:**
Scope creeps to include every Opus 4.6 feature, every new GSD capability, and every possible architectural improvement. The result: a months-long project delivering no value until completion, increasing system complexity, and potentially leaving the system in a worse state because each change interacts with every other change unpredictably.

**Why it happens:**
The PFS has been remarkably productive: 3 milestones in 6 days, 62+ requirements, 35+ plugins. The temptation is to "make it even better" by adopting everything new. But the system already works well -- it does not need a rewrite.

v1.2 was already an intelligence upgrade (resource discovery, context injection, accountability). v1.3 should be a targeted modernization, not a transformation.

**Warning signs:**
- Modernization plan has more than 5 phases
- Changes touching more than 50% of agent definitions in a single phase
- New abstractions being created rather than simplifying existing ones
- "While we're at it" additions to the scope
- Timeline exceeding the 2-day pace of previous milestones
- Creating new custom code to "modernize" (replacing one custom solution with another custom solution)

**How to avoid:**
1. **Apply the "real problem" test:** Has a plugin build ever failed because of the thing this change fixes? Has agent output quality suffered because of what this change addresses? If no to both, defer it
2. **Timebox** to match previous milestone pace (2 days)
3. **Prioritize ruthlessly:**
   - **Must have:** Update agent `model:` fields for Opus 4.6. Remove truly redundant custom code
   - **Should have:** Adopt new GSD features that directly replace custom workarounds
   - **Could have:** Persistent memory evaluation. Agent teams for cross-plugin operations
   - **Won't have this milestone:** Rewriting the stage pipeline. Restructuring agent contracts. New state management. Agent teams for the serial pipeline
4. **If a change requires modifying more than 5 files, it needs its own phase with dedicated canary testing**

**Phase to address:**
Phase 0 (Requirements) -- scope must be aggressively constrained before any implementation begins.

---

## v1.3 Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Updating agent model field without testing | Fast Opus 4.6 adoption | Agents may behave differently (reasoning style, output format, verbosity) | Never -- always test with canary plugin |
| Removing custom validators without replacement | Cleaner codebase | Loss of JUCE-specific validation (real-time safety, contract immutability, silent failure detection) | Never -- these are domain-critical safety nets |
| Using agent teams for all orchestration | Uniform architecture, "modern" approach | 5-10x token cost, file conflicts, unnecessary complexity, loss of subagent nesting | Only for genuinely parallel independent tasks in different directories |
| Skipping schema versioning | Faster iteration | Breaking changes cascade through all 6+ consumers silently | Only for truly additive optional fields with defaults |
| Relaxing token budgets | Richer agent context | More compaction events, higher costs, potential quality degradation | Only after measuring actual compaction behavior with before/after comparison |
| Adopting persistent memory before evaluating | Looks "intelligent" | Second unstructured knowledge system competing with structured resource discovery | Only after proving memory adds value above existing discovery system |
| Hardcoding new GSD paths | Works on this machine | Breaks on GSD update, different user, or different install location | Never -- use config or environment variables |
| Changing all agents simultaneously | Faster completion | Impossible to isolate which change caused a regression | Never -- one agent per commit with canary test |

## v1.3 Integration Gotchas

| Integration Point | Common Mistake | Correct Approach |
|-------------------|----------------|------------------|
| PFS agent `model:` vs GSD `model_profile` | Assuming GSD profile overrides PFS agent frontmatter | PFS agent frontmatter is authoritative for PFS agents; GSD profiles apply to GSD agents only. Document the boundary explicitly |
| PFS hooks + GSD phase lifecycle | Assuming GSD phases trigger PFS hooks in expected order | Verify hook trigger order by adding debug logging during testing phase |
| PFS schemas + new Opus 4.6 fields | Adding `memory`, `skills` to agents without updating schemas | Schema must be updated FIRST with optional fields + defaults, THEN agents can use new features |
| PFS resource discovery + GSD research | Running both PFS discovery AND GSD research phase, doubling context injection | Define ownership: PFS discovery for plugin workflows, GSD research for system-level improvements |
| PFS handoff protocol + GSD transitions | GSD auto-advancing while PFS expects mandatory `/clear` + next command | Configure GSD mode to respect PFS handoff protocol (manual transitions, explicit two-step) |
| PFS PreCompact + Opus 4.6 compaction | Assuming 1M context eliminates need for PreCompact | Keep PreCompact hook -- compaction still occurs, contracts still need preservation |
| PFS SessionStart + persistent memory | Agent memory loading at startup conflicting with SessionStart environment checks | Sequence matters: environment validation BEFORE agent memory is available |
| PFS `skill:` frontmatter + Claude Code Skills | PFS command `skill:` routing conflicting with Claude Code's skill discovery | Explicitly namespace PFS skill references to avoid collision |

## v1.3 "Looks Done But Isn't" Checklist

- [ ] **Agent model field updated:** Verify the agent ACTUALLY runs on specified model by checking Task invocation, not just frontmatter. Some models may not be available
- [ ] **Schema backward compatible:** Verify `additionalProperties: false` does not block new fields. Test with actual agent output from both old and new agents
- [ ] **Custom code removal verified:** The behavior the custom code provided is actually handled by the replacement. Run the same validation scenarios (real-time safety violations, contract immutability, silent failure detection)
- [ ] **GSD feature works within PFS orchestration:** Feature works in vanilla GSD AND within PFS's custom wrapping (pre/post processing, quality gates, handoffs)
- [ ] **Handoff protocol preserved:** `/clear` instruction and full plugin name still appear in every handoff message
- [ ] **Resource accountability intact:** `resources_consulted` field still populated in agent reports
- [ ] **Contract immutability enforced:** PostToolUse hook still blocks contract modification during Stages 1-4
- [ ] **Canary plugin passes:** Full `/implement --express` pipeline succeeds end-to-end on a simple plugin, not just individual agent invocations
- [ ] **No terminology confusion:** Plans and documentation use the correct primitive names (subagent vs agent team vs skill) per the terminology mapping
- [ ] **Token budgets maintained:** Research injection stays within 4K tokens, validation reports within 500 tokens, even with new features active

## v1.3 Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| P34: Custom code removed with PFS-specific behavior | MEDIUM | Git revert removal commit; re-classify code; re-add with documentation |
| P35: Agent contracts broken | HIGH | Revert agent + schema changes; restore from backup; re-run any in-progress plugins from last clean checkpoint |
| P36: Agent teams used for serial work | LOW | Revert to subagent pattern; no data loss, just wasted tokens |
| P37: Terminology confusion causing wrong primitive | LOW | Create terminology mapping; refactor plan to use correct names |
| P38: Context exhaustion from relaxed budgets | LOW | Restore previous token budget constants; re-run affected agent tasks |
| P39: GSD update breaks PFS | MEDIUM | Restore GSD backup; fix integration points one at a time against contract |
| P40: Plugin build regression | HIGH | Revert ALL modernization changes since last known-good state; re-introduce one at a time with canary testing |
| P41: Memory pollution | LOW | Delete `.claude/agent-memory/` directory; agents revert to stateless (resource discovery still works) |
| P42: State file format drift | MEDIUM | Restore from `.planning-v12-backup/`; write migration script; re-apply |
| P43: Scope creep | LOW | Re-read pitfalls; cut to must-haves only; timebox remaining work to 2 days |

## v1.3 Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| P34: Removing PFS-specific custom code | Phase 1 (Audit) | Every custom file classified as duplicate/extension/workaround with test case documenting preserved behavior |
| P35: Breaking agent contracts | Phase 2 (Agent Modernization) | Schema diff before/after shows only additive optional changes; canary plugin build passes |
| P36: Agent teams for serial work | Phase 1 (Audit) | Parallel suitability matrix created; stage pipeline explicitly marked "serial only" |
| P37: Terminology confusion | Phase 1 (Audit) | Terminology mapping document created and referenced in all subsequent phase plans |
| P38: Context window overconfidence | Phase 3 (Context) | Compaction event count measured before/after; token budgets verified unchanged |
| P39: GSD framework breaking PFS | Phase 1 (Audit) + Phase 4 (GSD Adoption) | Integration contract document listing all GSD dependencies; smoke tests passing |
| P40: Plugin build regression | ALL phases | Canary plugin passes after EVERY change |
| P41: Memory without hygiene | Phase 3 (Context) | Memory evaluation completed comparing value vs resource discovery; if adopted, hygiene strategy documented |
| P42: State file format drift | Phase 1 (Audit) | State files backed up; format validated against GSD expectations; migration script created if needed |
| P43: Over-engineering | Phase 0 (Requirements) | Scope document with must/should/could/won't prioritization; timebox enforced |

## v1.3 Pre-Implementation Checklist

Before starting v1.3 modernization:

- [ ] Backup all state files: `cp -r .planning/ .planning-v12-backup/`
- [ ] Verify GSD version: `cat ~/.claude/get-shit-done/VERSION` -- confirm 1.18.0
- [ ] Create terminology mapping (subagent vs agent team vs skill vs command)
- [ ] Create parallel suitability matrix for all PFS workflows
- [ ] Classify all custom code (duplicate / extension / workaround)
- [ ] Identify canary plugin for regression testing (simple effect, fast build)
- [ ] Map all GSD integration points (gsd-tools.js calls, template paths, config fields)
- [ ] Verify current agent frontmatter against Opus 4.6 frontmatter spec (what fields are new, what changed)
- [ ] Test current schemas against current agent output (establish known-good baseline)
- [ ] Set scope boundary: must-have vs should-have vs could-have vs won't-have

## v1.3 Sources

**Opus 4.6 and Agent Teams (HIGH confidence):**
- [Anthropic: Introducing Claude Opus 4.6](https://www.anthropic.com/news/claude-opus-4-6) -- 1M context beta, agent teams, improved reasoning
- [Claude Code Docs: Create custom subagents](https://code.claude.com/docs/en/sub-agents) -- Subagent configuration including memory, skills, hooks, permissionMode
- [Claude Code Docs: Orchestrate teams of Claude Code sessions](https://code.claude.com/docs/en/agent-teams) -- Agent teams architecture, limitations, comparison with subagents
- [Claude Code Docs: Extend Claude with skills](https://code.claude.com/docs/en/skills) -- Skills vs subagents vs commands, context: fork

**Agent System Modernization (MEDIUM confidence):**
- [TechCrunch: Anthropic releases Opus 4.6](https://techcrunch.com/2026/02/05/anthropic-releases-opus-4-6-with-new-agent-teams/) -- Feature overview and positioning
- [VentureBeat: Claude Opus 4.6 brings 1M token context and agent teams](https://venturebeat.com/technology/anthropics-claude-opus-4-6-brings-1m-token-context-and-agent-teams-to-take) -- Extended context capabilities
- [Vivek Haldar: Subagents, Commands and Skills Are Converging](https://www.vivekhaldar.com/articles/claude-code-subagents-commands-skills-converging/) -- Terminology convergence analysis

**Direct Codebase Analysis (HIGH confidence -- primary source):**
- `.claude/agents/*.md` -- All 11 agent definitions with frontmatter and prompt content
- `.claude/commands/*.md` -- All 40+ command definitions with skill routing
- `.claude/hooks/*.sh` -- All 3 shell hooks with timeout budgets and validation logic
- `.claude/hooks/validators/*.py` -- All 12 Python validators
- `.claude/scripts/*.py` -- All 6 custom scripts (discovery, injection, manifest, templates)
- `.claude/schemas/*.json` -- All schemas with `additionalProperties: false` constraints
- `.planning/PROJECT.md`, `.planning/STATE.md`, `.planning/MILESTONES.md` -- System history and current state
- `~/.claude/get-shit-done/` -- GSD 1.18.0 framework: workflows, templates, references, bin

---

*v1.3 Addendum Researched: 2026-02-08*
*Focus: Pitfalls specific to modernizing a 62+ requirement AI agent orchestration system with Opus 4.6 capabilities and GSD 1.18.0 alignment, while maintaining backward compatibility for 35+ production plugins*
