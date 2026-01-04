---
name: research
description: Deep investigation for complex JUCE problems
---

# /research

<routing>
  <instruction>
    Invoke the deep-research skill via the Skill tool.
  </instruction>

  <context_to_pass>
    Pass the user's topic/question as the research subject to the skill.
    Include any specific context from the conversation (plugin name, error messages, code snippets).
  </context_to_pass>

  <skill_responsibility>
    The deep-research skill handles:
    - Graduated research protocol execution (Level 1-3)
    - Local troubleshooting/ knowledge base search
    - Context7 JUCE documentation search
    - JUCE forum and GitHub investigation
    - Parallel research subagents (Level 3 only)
    - Decision menu presentation per checkpoint protocol
  </skill_responsibility>
</routing>

<state_management>
  <reads_from>
    <file path="troubleshooting/**/*.md">
      Searches local knowledge base for existing solutions (Level 1).
      Dual-indexed structure: by-plugin/ and by-symptom/.
    </file>
    <file path="troubleshooting/patterns/juce8-critical-patterns.md">
      May reference critical patterns during investigation.
    </file>
  </reads_from>

  <may_write_to>
    <file path="troubleshooting/[category]/[plugin]/[problem].md">
      If user chooses to document findings after Level 2-3 research.
      Handled by troubleshooting-docs skill (not deep-research itself).
    </file>
  </may_write_to>

  <integration>
    <command name="/doc-fix">
      Complementary: /doc-fix captures immediate solutions from conversation,
      /research investigates complex unknowns requiring external search.
    </command>
    <command name="/improve">
      May trigger research when implementing complex features or fixes.
    </command>
    <skill name="troubleshooting-docs">
      May be invoked after successful research to document findings.
    </skill>
  </integration>
</state_management>

<success_protocol>
After each level completion, the deep-research skill will:

1. Present findings summary (solutions, sources, confidence level)
2. Show decision menu per checkpoint protocol (see .claude/CLAUDE.md)
3. Typical options include:
   - Apply solution (recommended)
   - Review details
   - Escalate to next level (if applicable)
   - Document findings (Level 2-3 only)
   - Other
4. WAIT for user response before proceeding

The skill handles decision menu formatting and checkpoint protocol compliance.
</success_protocol>

<background_info>
## Purpose

The deep-research skill uses a graduated research protocol (3 levels) to efficiently find solutions
without over-researching simple problems. Auto-escalates based on confidence.

<graduated_protocol>
  <level number="1" duration="5-10min" model="Sonnet">
    <searches>
      - Local troubleshooting/ knowledge base
      - Context7 JUCE documentation (quick lookup)
    </searches>
    <output>Direct solution if confident match found</output>
    <escalates_if>No confident solution in local/cached sources</escalates_if>
  </level>

  <level number="2" duration="15-30min" model="Sonnet">
    <searches>
      - Context7 deep-dive (module-level documentation)
      - JUCE forum discussions
      - GitHub issues and solutions
    </searches>
    <output>Structured analysis with multiple solution options</output>
    <escalates_if>Multiple partial solutions requiring comparison, or novel problem</escalates_if>
  </level>

  <level number="3" duration="30-60min" model="Opus + extended thinking">
    <approach>Parallel research subagents (2-3 concurrent)</approach>
    <searches>
      - Comprehensive approach comparison
      - Academic papers (for DSP algorithms)
      - Advanced JUCE API patterns
    </searches>
    <output>Detailed report with implementation roadmap and trade-off analysis</output>
  </level>
</graduated_protocol>

## Examples

```bash
/research wavetable anti-aliasing implementation
/research how to use juce::dsp::Compressor
/research parameter not saving in DAW
/research CPU spikes when changing parameters
```


<usage_guidance>
  <appropriate_for>
    <case>Complex DSP algorithm questions (wavetable synthesis, anti-aliasing, etc.)</case>
    <case>Novel feature implementation research (no clear JUCE example)</case>
    <case>Performance optimization strategies (CPU profiling, buffer management)</case>
    <case>Advanced JUCE API usage (threading, real-time safety patterns)</case>
    <case>Unknown errors requiring multi-source investigation</case>
    <case>Architectural decisions (plugin structure, parameter management)</case>
  </appropriate_for>

  <not_appropriate_for>
    <case>Simple syntax errors - Fix directly, no research needed</case>
    <case>Known issues already in troubleshooting/ - Use grep or Level 1 will find immediately</case>
    <case>Basic JUCE API lookups - Use Context7 directly (faster)</case>
    <case>Build configuration issues - Check juce8-critical-patterns.md first</case>
  </not_appropriate_for>

  <efficiency_rationale>
    - 40% of problems solved at Level 1 (quick) - Known solutions
    - 40% of problems solved at Level 2 (moderate) - Forum/GitHub documented
    - 20% of problems require Level 3 (deep) - Novel/complex research
    - Knowledge base compounds: Level 3 today becomes Level 1 tomorrow
  </efficiency_rationale>
</usage_guidance>

<technical_implementation>
  <models>
    - Level 1-2: Sonnet (fast, sufficient for documented problems)
    - Level 3: Opus + extended thinking 15k budget (deep reasoning)
  </models>

  <timeout>Max 60 minutes (returns best findings if exceeded)</timeout>

  <parallel_investigation level="3">
    Spawns 2-3 concurrent research subagents, each investigating different approach.
    Main context synthesizes findings.
    Faster than serial (3 agents = 20 min, not 60 min).
  </parallel_investigation>
</technical_implementation>
</background_info>
