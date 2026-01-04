# Example Research Scenarios

## Scenario 1: WebView JavaScript Communication Freeze

**Problem:** User reports knobs freezing after WebView UI loads. Suspicion: race condition in message passing.

**Level 1 (Quick Check):**
1. Parse: "WebView", "JavaScript", "message passing", "race condition"
2. Search local: `Grep pattern:"WebView.*race|message.*freeze" path:troubleshooting/`
3. Result: Found `troubleshooting/gui-issues/webview-message-timing.md`
4. Confidence: HIGH (exact match with solution)
5. Decision: Present solution menu

**Outcome:** Solved in 3 minutes at Level 1.

---

## Scenario 2: APVTS Parameter Not Appearing in DAW

**Problem:** Parameter defined in code but not visible in DAW automation menu.

**Level 1 (Quick Check):**
1. Parse: "APVTS", "parameter", "automation", "DAW"
2. Search local: No exact match
3. Context7: Found `AudioProcessorValueTreeState` docs but unclear
4. Confidence: MEDIUM (API docs exist but problem not addressed)
5. Decision: Auto-escalate to Level 2

**Level 2 (Moderate Investigation):**
1. Context7 deep-dive: Cross-reference APVTS, PluginHostType, AudioProcessor
2. Forum search: Found thread "Parameters not automatable in Logic Pro"
3. GitHub: Found issue #542 "APVTS parameter visibility"
4. Synthesis: Parameter category metadata required for DAW visibility
5. Confidence: HIGH (multiple authoritative sources)
6. Decision: Present solution menu

**Outcome:** Solved in 18 minutes at Level 2.

---

## Scenario 3: Novel DSP Algorithm Implementation (IIR Filter Cascade)

**Problem:** Need to implement cascaded IIR filters with minimal phase distortion. No existing examples in codebase.

**Level 1 (Quick Check):**
1. Parse: "IIR filter", "cascade", "phase distortion", "DSP"
2. Search local: Generic DSP patterns but not cascaded IIR
3. Context7: Found `IIRFilter` class but no cascade examples
4. Confidence: LOW (API exists but novel use case)
5. Decision: Auto-escalate to Level 2

**Level 2 (Moderate Investigation):**
1. Context7: Deep-dive IIRFilter, ProcessorChain, dsp::ProcessSpec
2. Forum: Found discussions on filter design but not cascading
3. GitHub: No direct examples of cascaded IIR
4. Synthesis: API supports it but no precedent for minimal phase approach
5. Confidence: LOW (no clear precedent for novel requirement)
6. Decision: Auto-escalate to Level 3

**Level 3 (Deep Research):**
1. Model switch: Opus + extended thinking (15k budget)
2. Spawn 3 parallel subagents:
   - **Subagent A:** JUCE DSP architecture patterns (ProcessorChain, cascading)
   - **Subagent B:** DSP theory (IIR cascade phase distortion, correction techniques)
   - **Subagent C:** JUCE forum + GitHub deep-dive (related implementations)
3. Synthesis with extended thinking:
   - Subagent A: ProcessorChain supports cascading via template composition
   - Subagent B: Allpass filters for phase correction, minimum-phase design theory
   - Subagent C: Found forum post on filter cascade with phase compensation
4. Novel insight: Combine ProcessorChain with allpass phase compensator
5. Confidence: HIGH (novel solution synthesized from multiple authoritative sources)
6. Decision: Present comprehensive solution with implementation steps

**Outcome:** Solved in 42 minutes at Level 3 (20 min parallel subagents + 22 min synthesis).

---

## Scenario 4: Build Failure After JUCE 8 Migration

**Problem:** Project won't compile after upgrading from JUCE 7 to JUCE 8. Error: "AudioParameterFloat constructor ambiguous".

**Level 1 (Quick Check):**
1. Parse: "build failure", "JUCE 8 migration", "AudioParameterFloat", "constructor"
2. Search local: `Grep pattern:"AudioParameterFloat|JUCE 8" path:troubleshooting/build-failures/`
3. Result: Found `troubleshooting/api-usage/juce8-parameter-migration.md`
4. Content: JUCE 8 changed AudioParameterFloat constructor (breaking change)
5. Solution: Use new constructor signature with NormalisableRange
6. Confidence: HIGH (exact match in Required Reading)
7. Decision: Present solution menu

**Outcome:** Solved in 2 minutes at Level 1.

---

## Scenario 5: Unknown JUCE Component Behavior

**Problem:** User asks "How does LookAndFeel customization propagation work in JUCE?"

**Level 1 (Quick Check):**
1. Parse: "LookAndFeel", "customization", "propagation"
2. Search local: No specific docs on propagation mechanism
3. Context7: Found `LookAndFeel` class docs and examples
4. Confidence: HIGH (clear API documentation with examples)
5. Decision: Present explanation with API reference

**Outcome:** Answered in 4 minutes at Level 1.

---

## Key Patterns

### When Level 1 Succeeds:
- Exact match in local troubleshooting docs
- Clear API documentation in Context7
- Common problem with known solution
- **Time:** 2-10 minutes

### When Level 2 Needed:
- Local docs have partial/similar info
- API exists but usage unclear
- Problem needs cross-referencing multiple sources
- **Time:** 15-30 minutes

### When Level 3 Needed:
- Novel implementation (no precedent)
- Complex problem requiring multiple research angles
- Low confidence after Level 2
- Synthesis required from disparate sources
- **Time:** 30-60 minutes (20 min parallel + synthesis)

### Escalation Indicators:
- Confidence MEDIUM/LOW at current level
- "Novel" or "custom" in problem description
- Multiple failed searches at lower level
- Problem requires theoretical understanding + practical implementation
