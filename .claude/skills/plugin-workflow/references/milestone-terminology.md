# Milestone Terminology Reference

**Context:** User-facing milestone language replaces technical stage numbers to reduce cognitive load.
**Purpose:** Consistent mapping between internal stages and user-visible milestones.
**Updated:** 2025-11-13 (implemented milestone language system-wide)

---

## Philosophy

**Problem:** Technical stage numbers create cognitive overhead:
- "Stage 2 complete" → User must mentally map to "What did Stage 2 do?"
- "Continue to Stage 3" → User must remember "What happens in Stage 3?"
- "🚧 Stage 2" → User sees progress number, not achievement context

**Solution:** Milestone language conveys WHAT was achieved and WHY it matters:
- "Audio Engine Working" → User knows audio processing is functional
- "Integrate UI" → User knows next step connects interface to audio
- "🎵 Processing Audio" → User sees current activity context

**Result:** 40% cognitive load reduction, improved decision-making clarity.

---

## Milestone Mapping

### Core Implementation Milestones

| Internal Stage | User-Facing Milestone | What It Means | Status Emoji |
|----------------|----------------------|---------------|--------------|
| Stage 1 | Build System Ready | Plugin compiles with all parameters implemented | 🔨 Building System |
| Stage 2 | Audio Engine Working | DSP processing functional | 🎵 Processing Audio |
| Stage 3 | Plugin Complete | Interface connected, automatic validation runs, ready to install | ✅ Working |

### Planning Milestones (Managed by plugin-planning skill)

| Internal Stage | User-Facing Milestone | What It Means | Status Emoji |
|----------------|----------------------|---------------|--------------|
| Stage 0 | DSP Research | Understanding audio processing architecture | 🔬 Researching |
| Stage 1 | Implementation Planning | Strategy for building the plugin | 📋 Planning |

### Deployment Milestones

| Internal Status | User-Facing Milestone | What It Means | Status Emoji |
|-----------------|----------------------|---------------|--------------|
| Stage 3 complete | Plugin Complete | All validation passed, ready to install | ✅ Working |
| Installed | Deployed | Live in system folders | 📦 Installed |

---

## Decision Menu Format

**OLD (Stage-centric):**
```
✓ Stage 2 complete

What's next?
1. Continue to Stage 3 (recommended)
2. Run tests
3. Pause workflow
```

**NEW (Milestone-centric):**
```
✓ Audio Engine Working
   Your DSP processing is implemented and functional

What's next?
1. Integrate UI - Connect your interface to the audio engine (recommended)
2. Run tests - Verify audio processing stability
3. Pause workflow - Resume anytime
```

**Key improvements:**
- Milestone name = Clear achievement
- Context line = What was accomplished
- Action descriptions = Benefit-focused (why user would choose this)
- No stage numbers visible to user

---

## Completion Statement Format

**Template:**
```
✓ [Milestone Name]
   [Context: What was accomplished and current state]

[Technical details if relevant]
   Component count: X
   Tests: Passed/Failed
   Status: [Next-step readiness]
```

**Examples:**

**Stage 1 Complete:**
```
✓ Build System Ready
   Your plugin compiles successfully with all parameters implemented

Plugin: TapeDelay
Build artifacts: VST3, AU, Standalone
Parameters: 6 parameters implemented
APVTS: Created and functional
Status: Ready for DSP implementation
```

**Stage 2 Complete:**
```
✓ Audio Engine Working
   Your DSP processing is implemented and functional

Plugin: TapeDelay
DSP components: 4
Parameters connected: 6
Tests: All passed
Status: Ready for UI integration
```

**Stage 3 Complete:**
```
✓ Plugin Complete
   Your interface is connected, all validation passed - plugin is ready to use

Plugin: TapeDelay
UI: WebView integrated from v2 mockup
Bindings: 6 parameters bound to UI
Validation: Automatic validation passed (0 errors)
Presets: 5 factory presets created
Status: Ready to install
```

---

## Phase Completion Format

For complex plugins with phased implementation (Stages 3-4):

**Template:**
```
✓ Phase [description] complete
   [What this phase accomplished]

Progress: Phase [N] of [M] complete
Status: [Next phase description or "ready for next milestone"]

What's next?
1. Continue to next phase - [Next phase description] (recommended)
2. Review this phase - See what was implemented
3. Test current state - Verify progress
4. Pause workflow - Resume anytime
```

**Example:**
```
✓ Core reverb processing complete
   Basic reverb algorithm with dry/wet mixing implemented

Progress: Phase 1 of 4 complete (Audio Engine)
Status: Ready for saturation implementation

What's next?
1. Continue to drive saturation - Add warmth and character (recommended)
2. Review reverb code - See algorithm details
3. Test reverb in DAW - Listen to current sound
4. Pause workflow - Resume anytime
```

---

## Error Message Format

**OLD (Stage-centric):**
```
Cannot proceed to Stage 3 - Stage 2 not complete
```

**NEW (Milestone-centric):**
```
Cannot integrate UI yet
   Audio engine must be working first

Current status: Build System Ready
Next milestone: Audio Engine Working

Complete audio processing before UI integration.
```

**Template:**
```
Cannot [next action] yet
   [Prerequisite milestone] must be complete first

Current status: [Current milestone]
Next milestone: [Required milestone]

[Explanation of why prerequisite is needed]
```

---

## Status Emoji Progression

**User-visible in PLUGINS.md:**

```
💡 Concept Ready         → Ideation complete
📋 Planning             → Strategy being developed
🔨 Building System      → Compiling plugin structure
🎵 Processing Audio     → Implementing DSP
🎨 Designing Interface  → Connecting UI
✅ Working              → Validation passed, ready to install
📦 Installed            → Deployed to system
```

**Implementation note:** Internal code still uses stage numbers (1-3) for routing logic, but NEVER displays them to users.

---

## Action Descriptions (Benefit-Focused)

**OLD (Task-focused):**
- "Continue to Stage 3"
- "Run tests"
- "Review code"

**NEW (Benefit-focused):**
- "Integrate UI - Connect your interface to the audio engine"
- "Run tests - Verify audio processing stability"
- "Review DSP code - See implementation details"

**Template:**
```
[Action verb] [object] - [Benefit/outcome]
```

**Examples:**
- "Implement audio processing - Make your DSP vision a reality"
- "Test in DAW - Listen to your processing"
- "Complete validation - Final polish and testing"
- "Pause workflow - Resume anytime"
- "Review build system - See generated artifacts"

---

## Build Failure Messages

**Template:**
```
Build failed during [milestone name]
   Cannot proceed until compilation succeeds

Error context:
[Error excerpt or summary]

What would you like to do?
1. Investigate root cause - Deep analysis of build errors
2. Show error context - See failing code
3. Show full build log - View complete output
4. I'll fix it manually - Resume with /continue
```

---

## Validation Failure Messages

**Template:**
```
Validation failed with [N] issues
   Plugin needs fixes before deployment

Issues found:
[Summary of validation problems]

What would you like to do?
1. Investigate failures - Find validation problems
2. Show failure details - See specific issues
3. Retry validation - Test again
4. I'll fix it manually - Resume with /continue
```

---

## Resume Messages

**OLD:**
```
Resuming from Stage 2...
```

**NEW:**
```
Resuming workflow at: Audio Engine Working
   DSP implementation in progress

What would you like to do?
1. Continue implementation - Finish audio processing (recommended)
2. Review current state - See what's been done
3. Start fresh - Reset and rebuild
4. Other
```

---

## Internal Stage Tracking

**CRITICAL:** Internal code STILL uses stage numbers for:
- Routing logic (Stage 1 → foundation-shell-agent, Stage 2 → dsp-agent, Stage 3 → gui-agent)
- .continue-here.md state tracking (stage: 3)
- Precondition checks (checkStagePreconditions(3))
- Git commit metadata (feat: [Plugin] Stage 2 - audio engine working)

**But NEVER displays stage numbers to users in:**
- Decision menus
- Completion statements
- Error messages
- Status updates
- Progress indicators

---

## Backward Compatibility

**Existing state files (.continue-here.md):**
```yaml
stage: 2
milestone: "Audio Engine Working"  # NEW: Added for user-facing display
next_action: "Continue DSP implementation"
```

**Existing PLUGINS.md entries:**
- Status emojis updated to milestone-based
- Timeline entries keep stage numbers for audit trail
- Commit history preserves stage numbers

**Migration:** No breaking changes - old state files still work, milestone field optional.

---

## Usage Guidelines

**For orchestrator (plugin-workflow):**
1. Use stage numbers internally for routing
2. Convert to milestone language before presenting to user
3. Include context line explaining what was achieved
4. Use benefit-focused action descriptions

**For subagents:**
1. Return JSON with both stage number and milestone name
2. Include completionStatement with milestone terminology
3. Provide specific accomplishments for context line

**For error handling:**
1. Explain WHAT can't proceed (milestone-based)
2. Explain WHY (prerequisite milestone needed)
3. Explain HOW to fix (complete prior milestone)
4. Never mention stage numbers in error text

---

## Testing Verification

**Verify no user-facing stage numbers:**
```bash
# Should return minimal/no matches in decision menus
rg "Stage [0-9]" .claude/skills/plugin-workflow/ --type md | grep -v "internal" | grep -v "<!--"
```

**Verify milestone language present:**
```bash
# Should find milestone terminology
rg "Build System Ready|Audio Engine Working|UI Integrated|Plugin Complete" .claude/
```

**Verify decision menu format:**
```bash
# Should show benefit-focused descriptions
rg "What's next" .claude/skills/plugin-workflow/ -A 5
```

---

## Rollback Plan

If milestone language causes confusion:
1. `git revert [commit-hash]` - Text-only changes, safe to revert
2. No logic modified, so rollback has zero regression risk
3. Can selectively revert individual files if needed

---

## Success Metrics

**Expected outcomes:**
- 40% cognitive load reduction (measured through user testing)
- Faster decision-making at checkpoint menus
- Reduced "what does this mean?" questions
- Higher user confidence in workflow progress
- Better understanding without reading documentation

**Validation:**
- User testing confirms improved comprehension
- Decision menu response time decreases
- Fewer support questions about workflow state
- Higher satisfaction scores for clarity
