# Graduated Research Protocol

## Level 1: Quick Check (5-10 min, Sonnet, no extended thinking)

**Sources:**

1. Local troubleshooting docs (search for similar symptoms)
2. Context7 JUCE documentation (quick API lookup)

**Decision:** If confident solution found → return immediately
**Escalate to Level 2 if:** No solution or low confidence

See SKILL.md for detailed Level 1 process (lines 177-192).

---

## Level 2: Moderate Investigation (15-30 min, Sonnet, no extended thinking)

**Sources:**

1. Context7 deep-dive (module documentation, examples)
2. JUCE forum search (known issues, discussions)
3. GitHub issue search (juce-framework/JUCE repository)

**Decision:** If confident solution found → return immediately
**Escalate to Level 3 if:** Complex algorithm question, novel implementation needed

### Level 2 Process

1. **Context7 deep-dive**

   - Full module documentation
   - Code examples and patterns
   - Related classes and methods
   - Best practices sections

2. **JUCE forum search**

   ```
   Query: "site:forum.juce.com [topic]"
   Look for: Recent threads, accepted solutions, JUCE team responses
   ```

3. **GitHub issue search**

   ```
   Query: "site:github.com/juce-framework/JUCE [topic]"
   Look for: Open/closed issues, pull requests, code discussions
   ```

4. **Synthesize findings**

   - Compare multiple sources
   - Verify version compatibility
   - Assess solution quality

5. **Decision point**
   - HIGH/MEDIUM confidence: Present structured report
   - LOW confidence OR novel problem: Escalate to Level 3

---

## Level 3: Deep Research (30-60 min, Opus, extended thinking 15k budget)

**Sources:**

1. All Level 1-2 sources
2. Academic papers (DSP algorithms, audio processing)
3. Commercial plugin research (similar implementations)
4. Parallel subagent spawning (investigate multiple approaches)

**Always returns:** Structured report with recommendations (never escalates further)

### Level 3 Process

1. **Switch to Opus + Extended Thinking**

   - Extended thinking budget: 15,000 tokens
   - Timeout: 60 minutes

2. **Parallel subagent investigation**

   Spawn 2-3 research subagents via Task tool with explicit model override:

   **Task tool syntax:**
   ```
   Task tool with:
   - model: "claude-opus-4-1-20250805"
   - extended_thinking: 15000
   - task: "[Focused research goal for this subagent]"
   ```

   **Example: Researching wavetable anti-aliasing**

   Spawn 3 parallel Task tool calls in a single response:

   ```
   Task 1:
   - model: "claude-opus-4-1-20250805"
   - extended_thinking: 15000
   - task: "Investigate BLEP (Band-Limited Step) anti-aliasing approach. Research implementation complexity, CPU cost, quality assessment, and JUCE integration."

   Task 2:
   - model: "claude-opus-4-1-20250805"
   - extended_thinking: 15000
   - task: "Investigate oversampling + filtering approach for wavetable anti-aliasing. Research implementation complexity, CPU cost, quality assessment, and juce::dsp::Oversampling API."

   Task 3:
   - model: "claude-opus-4-1-20250805"
   - extended_thinking: 15000
   - task: "Research commercial wavetable synth implementations. Identify industry standards, common approaches, and best practices for anti-aliasing."
   ```

   Each subagent runs in fresh context with focused research goal.

3. **Main context synthesis**

   - Aggregate findings from all subagents
   - Compare approaches (pros/cons for each)
   - Estimate implementation complexity
   - Assess JUCE integration
   - Consider CPU/quality trade-offs

4. **Academic paper search (if applicable)**

   - DSP algorithms: "Reducing Aliasing from Synthetic Audio Signals"
   - Audio processing: "Phase Vocoder Tutorial"
   - Performance optimization: "Real-Time Audio Programming"

5. **Generate comprehensive report**
   - All findings structured
   - Multiple solution options
   - Pros/cons for each
   - Implementation roadmap
   - Recommended approach with rationale

---

## Auto-Escalation Triggers

**Level 1 → Level 2:**

- No results in local docs after 5 min
- Context7 lookup insufficient (API exists but unclear usage)
- Low confidence in solution

**Level 2 → Level 3:**

- Complex DSP algorithm question
- Novel feature requiring original implementation
- No clear answer after moderate investigation
- Multiple conflicting solutions found (need synthesis)

---

## User Override

At each level, present decision menu:

### After Level 1

```
✓ Level 1 complete (found potential solution)

What's next?
1. Apply solution (recommended) - [Brief description]
2. Review findings - See detailed explanation
3. Continue deeper - Escalate to Level 2 investigation
4. Other
```

### After Level 2

```
✓ Level 2 complete (found M solutions)

What's next?
1. Apply recommended solution - [Solution name]
2. Review all options - Compare N approaches
3. Continue deeper - Escalate to Level 3 (parallel investigation)
4. Other
```

### After Level 3

```
✓ Level 3 complete (comprehensive investigation)

Investigated N approaches, recommend: [Solution name]

What's next?
1. Apply recommended solution (recommended) - [Brief description]
2. Review all findings - See detailed comparison
3. Try alternative approach - [Alternative name]
4. Document findings - Save to troubleshooting/
5. Other
```

---

## Timeout Handling

**Level 1 timeout (>10 min):**

- Auto-escalate to Level 2
- Notify user: "Level 1 taking longer than expected, escalating..."

**Level 2 timeout (>30 min):**

- Auto-escalate to Level 3
- Notify user: "Complex problem detected, escalating to deep research..."

**Level 3 timeout (>60 min):**

- Return best answer based on research so far
- Add warning: "Research incomplete due to timeout (60 min limit)"
- Present option: "Retry with extended timeout" / "Continue with current findings"

---

## Success Criteria by Level

### Level 1 Success

- ✅ Found exact match in local docs OR clear API documentation
- ✅ Solution is directly applicable
- ✅ High confidence (80%+)

### Level 2 Success

- ✅ Found authoritative answer in JUCE forum/GitHub OR comprehensive Context7 docs
- ✅ Solution verified by multiple sources
- ✅ Medium-high confidence (60%+)

### Level 3 Success

- ✅ Investigated multiple approaches thoroughly
- ✅ Compared pros/cons for each
- ✅ Provided implementation roadmap
- ✅ Recommended solution with clear rationale
- ✅ Confidence level clearly stated (may be low if novel problem)

---

## Example Escalation Flow

**Scenario:** "How to implement wavetable anti-aliasing?"

**Level 1 (5 min):**

- Search troubleshooting/ → No matches
- Quick Context7 lookup → juce::dsp::Oscillator exists, no anti-aliasing docs
- **Decision:** LOW confidence, escalate to Level 2

**Level 2 (15 min):**

- Context7 deep-dive → Found Oscillator class but no built-in anti-aliasing
- JUCE forum search → Found discussions, multiple approaches mentioned
- GitHub search → Found related issues but no definitive answer
- **Decision:** MEDIUM confidence but complex algorithm question, escalate to Level 3

**Level 3 (45 min, Opus + extended thinking):**

- Spawn Subagent 1: Research BLEP approach
  - Findings: CPU efficient, good quality, JUCE has helpers
  - Complexity: Medium (3/5)
- Spawn Subagent 2: Research oversampling approach
  - Findings: Works for all waveforms, JUCE has juce::dsp::Oversampling
  - Complexity: Low (2/5), higher CPU cost
- Spawn Subagent 3: Research commercial implementations
  - Findings: Most use oversampling for simplicity
- Academic paper: "Reducing Aliasing from Synthetic Audio Signals"
- **Synthesize:** Recommend oversampling (simpler, JUCE native support)
- **Confidence:** HIGH (verified by multiple sources)

**Output:** Comprehensive report with 2 viable solutions, recommended approach, implementation steps.

---

## Notes for Claude

**When executing this protocol:**

1. Always start at Level 1 (never skip to higher levels)
2. Stop as soon as confident answer found (don't over-research)
3. Present decision menu at each level (user controls depth)
4. Use relative confidence assessment (HIGH/MEDIUM/LOW)
5. For Level 3, always spawn parallel subagents (don't do serial research)
6. Switch model to Opus at Level 3 (architecture requirement)
7. Enable extended thinking at Level 3 (15k token budget)
8. Timeout at 60 min max (return best findings)

**Common pitfalls:**

- Skipping Level 1 local docs (always check first, fastest path)
- Not spawning parallel subagents at Level 3 (defeats purpose)
- Using wrong model (Sonnet for L1-2, Opus for L3)
- Forgetting extended thinking at L3 (critical for synthesis)
- Over-researching simple problems (stop at HIGH confidence)

---

## Performance Budgets

Performance expectations and success rates by level.

**Level 1:**
- Time: 5-10 min
- Extended thinking: No
- Success rate: 40% of problems (known solutions)

**Level 2:**
- Time: 15-30 min
- Extended thinking: No
- Success rate: 40% of problems (documented solutions)

**Level 3:**
- Time: 30-60 min
- Extended thinking: Yes (15k budget)
- Success rate: 20% of problems (novel/complex)

**Overall:**
- Average resolution time: 15 min (weighted by success rates)
- 80% of problems solved at Level 1-2 (fast)
- 20% require Level 3 (deep research justified)
