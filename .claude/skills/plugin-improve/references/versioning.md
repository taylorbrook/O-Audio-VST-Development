# Semantic Versioning Decision Tree

## Version Bump Logic

```
Request analysis:
├─ Breaking change detected? → MAJOR (X.0.0)
│  ├─ Parameter ID changes
│  ├─ API signature changes
│  ├─ Incompatible state format
│  └─ Removed features
│
├─ New feature added? → MINOR (0.X.0)
│  ├─ New parameters
│  ├─ New capabilities
│  ├─ Enhanced features
│  └─ Backward compatible changes
│
└─ Bug fix only? → PATCH (0.0.X)
   ├─ Fixed crashes
   ├─ Fixed incorrect behavior
   ├─ Performance fixes
   └─ No new features
```

## Decision Table

| Change Type | Examples | Version Bump |
|------------|----------|--------------|
| Bug fix | Crash fix, calculation error, UI glitch | PATCH (0.0.X) |
| New feature | New parameter, new algorithm option | MINOR (0.X.0) |
| Enhancement | Improved UI, better performance | MINOR (0.X.0) |
| Breaking change | Parameter rename, API change | MAJOR (X.0.0) |
| Refactoring | Code cleanup, no behavior change | PATCH (0.0.X) |

## Breaking Change Detection

A change is **breaking** if:

1. **Parameter IDs change** (automation will break)
2. **Parameter ranges change** (saved values may be invalid)
3. **State format changes** (old presets won't load)
4. **Features removed** (users lose functionality)
5. **API signatures change** (if used as library)

If unsure, prefer **MINOR** over **MAJOR** (conservative approach).

## Version String Format

`MAJOR.MINOR.PATCH` where:
- **MAJOR:** Incompatible changes
- **MINOR:** Backward compatible features
- **PATCH:** Backward compatible fixes

Examples:
- `1.0.0` → `1.0.1`: Bug fix (PATCH)
- `1.0.1` → `1.1.0`: New feature (MINOR)
- `1.1.0` → `2.0.0`: Breaking change (MAJOR)

## Special Cases

### Pre-1.0.0 Versions
- Major version 0 (0.X.Y) signals initial development
- Breaking changes can use MINOR bumps (0.X.0)
- Stability not guaranteed

### Multiple Change Types
- If both feature and bug fix → MINOR
- If both feature and breaking change → MAJOR
- Always use highest severity bump
