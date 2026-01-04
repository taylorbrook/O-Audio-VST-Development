# Regression Testing Protocol

## RegressionReport Interface

```typescript
interface RegressionReport {
  plugin: string;
  baselineVersion: string;
  newVersion: string;
  timestamp: string;

  summary: {
    totalTests: number;
    passed: number;
    failed: number;
    warnings: number;
  };

  results: {
    buildTest: {
      passed: boolean;
      errors?: string[];
    };

    loadTest: {
      passed: boolean;
      loadTime: number;
      errors?: string[];
    };

    parameterTest: {
      passed: boolean;
      changedParameters?: string[];
      missingParameters?: string[];
      newParameters?: string[];
    };

    audioTest: {
      passed: boolean;
      methodology: "pluginval" | "manual" | "automated";
      results?: {
        testName: string;
        passed: boolean;
        details?: string;
      }[];
    };
  };

  regressions?: {
    parameter: string;
    issue: string;
    severity: "critical" | "warning";
    baselineBehavior: string;
    newBehavior: string;
  }[];

  recommendation: "deploy" | "review" | "rollback";
  notes?: string;
}
```

## Test Execution Flow

1. **Baseline backup must exist** at `backups/${PLUGIN_NAME}/v${BASELINE_VERSION}/`
2. **Invoke plugin-testing skill** with baseline and new versions
3. **Collect test results** in RegressionReport format
4. **Analyze failures:**
   - Critical regressions → Recommend rollback
   - Warnings → Recommend review
   - All pass → Recommend deploy

## Comparison Methodology

### Build Test
- Compile both versions
- Verify no new warnings/errors

### Load Test
- Load plugin in host
- Measure load time (performance regression check)

### Parameter Test
- Compare APVTS parameter lists
- Detect: added, removed, changed parameters

### Audio Test
- Use pluginval if available
- Compare DSP output for same input
- Flag if output differs (potential regression)

## Failure Handling

### Critical Regressions
```
❌ CRITICAL REGRESSIONS DETECTED

[List of regressions with severity: critical]

Recommendation: ROLLBACK

Options:
1. Rollback to v${BASELINE_VERSION} - Undo all changes
2. Investigate failures - Debug before deploying
3. Deploy anyway - Override safety check (not recommended)

Choose (1-3): _
```

### Warnings Only
```
⚠️  WARNINGS DETECTED

[List of regressions with severity: warning]

These may be expected (e.g., intentional behavior changes).

Recommendation: REVIEW

Options:
1. Review detailed report - See full test results
2. Deploy anyway - Warnings acceptable
3. Rollback - Too risky

Choose (1-3): _
```

### All Tests Pass
```
✓ All regression tests passed

Baseline: v${BASELINE_VERSION} → New: v${NEW_VERSION}
- Build: ✓ Pass
- Load: ✓ Pass
- Parameters: ✓ No unexpected changes
- Audio: ✓ Pass

Recommendation: DEPLOY

Proceed to Phase 6 (Git Workflow)?
```

## Integration with plugin-testing

If `.claude/skills/plugin-testing/SKILL.md` exists:

1. **Invoke skill** with baseline and comparison versions
2. **Skill returns** RegressionReport JSON
3. **Parse report** and check recommendation field
4. **Present results** to user with decision menu

If plugin-testing not available:
- Skip regression testing
- Warn user: "Regression testing skipped (plugin-testing skill not installed)"
