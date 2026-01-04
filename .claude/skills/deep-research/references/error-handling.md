# Error Handling Patterns

Error handling, timeout management, and fallback strategies for deep-research skill.

## Contents

- [Timeout Handling](#timeout-handling)
- [No Solution Found](#no-solution-found)
- [Subagent Failure](#subagent-failure)
- [WebSearch Unavailable](#websearch-unavailable)

---

## Timeout Handling

**Timeout (>60 min):**

```
⚠️ Research timeout (60 min limit)

Returning best findings based on investigation so far:
[Partial findings]

What's next?
1. Use current findings - Proceed with partial answer
2. Retry with extended timeout - Continue research
3. Other
```

---

## No Solution Found

**Research exhausted (Level 3 complete, no definitive solution):**

```
Research exhausted (Level 3 complete, no definitive solution)

Attempted:
✓ Local troubleshooting docs (0 matches)
✓ Context7 JUCE documentation (API exists but undocumented)
✓ JUCE forum + GitHub (no clear consensus)
✓ Parallel investigation (3 approaches, all have significant drawbacks)

Recommendations:
1. Post to JUCE forum with detailed investigation
2. Try experimental approach: [Suggestion]
3. Consider alternative feature: [Workaround]

I can help formulate forum post if needed.
```

---

## Subagent Failure

**Subagent failure (Level 3 only):**

```
⚠️ Subagent [N] failed to complete research

Proceeding with findings from other subagents (N-1 completed)...
```

**Action**: Continue with available findings, note partial investigation in report.

---

## WebSearch Unavailable

**Level 2 proceeds with Context7 only:**

```
⚠️ Web search unavailable

Proceeding with Context7 JUCE documentation only.
Results may be incomplete for community knowledge.
```

**Action**: Complete Level 2 using available sources, note limitation in report, offer escalation to Level 3.
