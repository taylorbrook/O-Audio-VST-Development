# Deep Research Integrations

Integration details for deep-research skill with other Plugin Freedom System components.

## Contents

- [Integration with troubleshoot-agent](#integration-with-troubleshoot-agent)
- [Integration with troubleshooting-docs](#integration-with-troubleshooting-docs)

---

## Integration with troubleshoot-agent

**troubleshoot-agent Level 4:**

When troubleshoot-agent exhausts Levels 0-3, it invokes deep-research:

```markdown
## troubleshoot-agent.md - Level 4

If Levels 0-3 insufficient, escalate to deep-research skill:

"I need to investigate this more deeply. Invoking deep-research skill..."

[Invoke deep-research with problem context]

deep-research handles:

- Graduated research protocol (Levels 1-3)
- Parallel investigation (Level 3)
- Extended thinking budget
- Returns structured report with recommendations
```

**Integration flow:**

1. troubleshoot-agent detects complex problem (Level 3 insufficient)
2. Invokes deep-research skill
3. deep-research starts at Level 1 (may escalate)
4. Returns structured report to troubleshoot-agent
5. troubleshoot-agent presents findings to user

---

## Integration with troubleshooting-docs

**NOTE**: Documentation should occur AFTER plugin-improve successfully applies the solution, not during deep-research. deep-research is read-only and does not implement changes.

**Proper flow:**
1. deep-research investigates and returns findings
2. User selects "Apply solution" → plugin-improve skill invoked
3. plugin-improve implements changes and tests
4. After successful implementation, plugin-improve suggests documentation
5. User chooses to document → troubleshooting-docs skill invoked

**Auto-suggest documentation pattern** (for plugin-improve, not deep-research):

```
✓ Solution applied successfully

This was a complex problem (Level N research). Document for future reference?

1. Yes - Create troubleshooting doc (recommended)
2. No - Skip documentation
3. Other
```

If user chooses "Yes":

- Invoke troubleshooting-docs skill
- Pass research report + solution as context
- Creates dual-indexed documentation
- Future Level 1 searches will find it instantly

**The feedback loop:**

1. Level 3 research solves complex problem
2. plugin-improve applies solution
3. Document solution → troubleshooting/
4. Similar problem occurs → Level 1 finds it instantly
5. Knowledge compounds, research gets faster over time
