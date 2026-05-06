# Missing Agent Specifications

Specifications for agents identified as gaps in the workflow audit (01-01-AUDIT.md). Each spec is complete enough to implement as a new skill.

## Agent Specifications

---

### music-theory-agent

**Priority:** HIGH
**Gap Addressed:** Multiple plugins need JI/temperament math; current ad-hoc approach is inefficient

**Purpose:**
Provides music theory calculations and guidance for tuning, temperament, intervals, and harmonic relationships. Consulted during DSP implementation for plugins involving pitch, harmony, or intonation.

**Input Contract:**

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "music-theory-agent.input",
  "type": "object",
  "required": ["query_type"],
  "properties": {
    "query_type": {
      "type": "string",
      "enum": [
        "interval_ratio",
        "temperament_frequencies",
        "chord_voicing",
        "scale_degrees",
        "tuning_table",
        "cents_conversion",
        "harmonic_series"
      ],
      "description": "Type of music theory calculation needed"
    },
    "parameters": {
      "type": "object",
      "description": "Query-specific parameters",
      "properties": {
        "root_frequency": {
          "type": "number",
          "description": "Base frequency in Hz (default: 440)"
        },
        "interval": {
          "type": "string",
          "description": "Interval name (e.g., 'perfect_fifth', 'major_third')"
        },
        "temperament": {
          "type": "string",
          "enum": ["just", "equal", "pythagorean", "meantone", "werckmeister"],
          "description": "Tuning system to use"
        },
        "scale": {
          "type": "string",
          "description": "Scale name (e.g., 'major', 'minor', 'dorian')"
        },
        "chord_type": {
          "type": "string",
          "description": "Chord type (e.g., 'major', 'minor', 'dominant7')"
        }
      }
    },
    "context": {
      "type": "object",
      "properties": {
        "plugin_name": { "type": "string" },
        "use_case": { "type": "string" }
      }
    }
  },
  "additionalProperties": false
}
```

**Output Contract:**

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "music-theory-agent.output",
  "type": "object",
  "required": ["status", "result"],
  "properties": {
    "status": {
      "type": "string",
      "enum": ["success", "error"]
    },
    "result": {
      "type": "object",
      "description": "Query-specific result data",
      "properties": {
        "frequencies": {
          "type": "array",
          "items": { "type": "number" }
        },
        "ratios": {
          "type": "array",
          "items": { "type": "string" }
        },
        "cents": {
          "type": "array",
          "items": { "type": "number" }
        },
        "midi_notes": {
          "type": "array",
          "items": { "type": "integer" }
        }
      }
    },
    "explanation": {
      "type": "string",
      "description": "Human-readable explanation of the calculation"
    },
    "code_snippet": {
      "type": "string",
      "description": "Optional C++ code implementing the calculation"
    },
    "formula": {
      "type": "string",
      "description": "Mathematical formula used"
    },
    "error": {
      "type": "object",
      "properties": {
        "code": { "type": "string" },
        "message": { "type": "string" }
      }
    }
  },
  "if": {
    "properties": { "status": { "const": "error" } }
  },
  "then": {
    "required": ["status", "error"]
  },
  "additionalProperties": false
}
```

**Boundaries:**

*This Agent DOES:*
- Calculate just intonation ratios for any interval
- Generate frequency tables for various temperaments
- Provide chord voicing suggestions with frequencies
- Explain scale/mode relationships
- Generate C++ code snippets for pitch calculations
- Convert between cents, ratios, and frequencies
- Provide harmonic series calculations

*This Agent DOES NOT:*
- Implement DSP code directly (use dsp-agent)
- Make architectural decisions (use plugin-planning)
- Handle audio buffer processing
- Design user interfaces
- Modify source files
- Execute builds

**Handoffs:**
- Receives from: dsp-agent (for tuning calculations during Stage 2)
- Outputs to: dsp-agent (calculation results and code snippets)
- May invoke: deep-research (for novel tuning systems)

**Tools:**
- Read (reference materials)
- WebSearch (external tuning resources)

---

### aesthetics-agent

**Priority:** MEDIUM
**Gap Addressed:** Visual quality could improve; aesthetic-dreaming partially addresses but lacks systematic design reasoning

**Purpose:**
Provides design guidance for plugin UI aesthetics, visual consistency, and user experience. Consulted during GUI implementation for color schemes, layout principles, and professional polish.

**Input Contract:**

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "aesthetics-agent.input",
  "type": "object",
  "required": ["query_type"],
  "properties": {
    "query_type": {
      "type": "string",
      "enum": [
        "color_scheme",
        "layout_review",
        "typography",
        "visual_hierarchy",
        "polish_suggestions",
        "accessibility_check",
        "consistency_audit"
      ],
      "description": "Type of aesthetic guidance needed"
    },
    "context": {
      "type": "object",
      "properties": {
        "plugin_name": { "type": "string" },
        "plugin_type": {
          "type": "string",
          "enum": ["effect", "instrument", "utility", "analyzer"]
        },
        "existing_mockup": { "type": "string" },
        "brand_guidelines": { "type": "string" },
        "target_audience": { "type": "string" },
        "reference_plugins": {
          "type": "array",
          "items": { "type": "string" }
        }
      }
    },
    "constraints": {
      "type": "object",
      "properties": {
        "width_px": { "type": "integer" },
        "height_px": { "type": "integer" },
        "parameter_count": { "type": "integer" },
        "must_include_meters": { "type": "boolean" }
      }
    }
  },
  "additionalProperties": false
}
```

**Output Contract:**

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "aesthetics-agent.output",
  "type": "object",
  "required": ["status", "recommendations"],
  "properties": {
    "status": {
      "type": "string",
      "enum": ["success", "error"]
    },
    "recommendations": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["category", "suggestion", "priority"],
        "properties": {
          "category": {
            "type": "string",
            "enum": ["color", "layout", "typography", "spacing", "contrast", "hierarchy"]
          },
          "suggestion": { "type": "string" },
          "rationale": { "type": "string" },
          "priority": {
            "type": "string",
            "enum": ["high", "medium", "low"]
          },
          "before_after": {
            "type": "object",
            "properties": {
              "before": { "type": "string" },
              "after": { "type": "string" }
            }
          }
        }
      }
    },
    "color_palette": {
      "type": "array",
      "items": { "type": "string" },
      "description": "Recommended hex colors if applicable"
    },
    "typography": {
      "type": "object",
      "properties": {
        "heading_font": { "type": "string" },
        "body_font": { "type": "string" },
        "sizes": {
          "type": "object",
          "properties": {
            "heading": { "type": "string" },
            "label": { "type": "string" },
            "value": { "type": "string" }
          }
        }
      }
    },
    "accessibility_score": {
      "type": "object",
      "properties": {
        "contrast_ratio": { "type": "number" },
        "wcag_level": {
          "type": "string",
          "enum": ["AAA", "AA", "A", "fail"]
        },
        "issues": {
          "type": "array",
          "items": { "type": "string" }
        }
      }
    },
    "error": {
      "type": "object",
      "properties": {
        "code": { "type": "string" },
        "message": { "type": "string" }
      }
    }
  },
  "if": {
    "properties": { "status": { "const": "error" } }
  },
  "then": {
    "required": ["status", "error"]
  },
  "additionalProperties": false
}
```

**Boundaries:**

*This Agent DOES:*
- Recommend color schemes based on plugin purpose
- Review layouts for visual balance
- Suggest typography improvements
- Identify visual hierarchy issues
- Provide polish suggestions for professional look
- Check accessibility (contrast ratios, color blindness)
- Audit consistency across UI elements

*This Agent DOES NOT:*
- Implement CSS/HTML code (use gui-agent)
- Create mockup images (use ui-mockup)
- Make functional decisions
- Override user's creative vision
- Modify source files directly

**Handoffs:**
- Receives from: gui-agent (for design review during Stage 3)
- Receives from: ui-mockup (for pre-implementation review)
- Outputs to: gui-agent (recommendations to implement)

**Tools:**
- Read (mockup files, design specs)
- WebSearch (design inspiration, best practices)

---

### performance-profiling-agent

**Priority:** LOW
**Gap Addressed:** Performance optimization is important but current manual approach is functional

**Purpose:**
Provides CPU/memory profiling analysis and optimization recommendations for audio plugins. Consulted during Stage 4 (Polish) for performance-critical code paths.

**Input Contract:**

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "performance-profiling-agent.input",
  "type": "object",
  "required": ["plugin_name", "profile_type"],
  "properties": {
    "plugin_name": {
      "type": "string",
      "pattern": "^[A-Z][a-zA-Z0-9-]*$"
    },
    "profile_type": {
      "type": "string",
      "enum": ["cpu", "memory", "latency", "full"],
      "description": "Type of profiling to perform"
    },
    "context": {
      "type": "object",
      "properties": {
        "sample_rate": { "type": "integer" },
        "buffer_size": { "type": "integer" },
        "channel_count": { "type": "integer" },
        "target_cpu_percent": { "type": "number" }
      }
    },
    "hotspot_hint": {
      "type": "string",
      "description": "Suspected performance bottleneck location"
    }
  },
  "additionalProperties": false
}
```

**Output Contract:**

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "performance-profiling-agent.output",
  "type": "object",
  "required": ["status"],
  "properties": {
    "status": {
      "type": "string",
      "enum": ["success", "error"]
    },
    "metrics": {
      "type": "object",
      "properties": {
        "cpu_percent": { "type": "number" },
        "memory_mb": { "type": "number" },
        "latency_samples": { "type": "integer" },
        "allocations_per_block": { "type": "integer" }
      }
    },
    "hotspots": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "location": { "type": "string" },
          "cpu_percent": { "type": "number" },
          "recommendation": { "type": "string" },
          "severity": {
            "type": "string",
            "enum": ["critical", "warning", "info"]
          }
        }
      }
    },
    "optimizations": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "description": { "type": "string" },
          "expected_improvement": { "type": "string" },
          "code_change": { "type": "string" }
        }
      }
    },
    "real_time_safe": {
      "type": "boolean",
      "description": "Whether processBlock is real-time safe"
    },
    "violations": {
      "type": "array",
      "items": { "type": "string" },
      "description": "Real-time safety violations found"
    },
    "error": {
      "type": "object",
      "properties": {
        "code": { "type": "string" },
        "message": { "type": "string" }
      }
    }
  },
  "if": {
    "properties": { "status": { "const": "error" } }
  },
  "then": {
    "required": ["status", "error"]
  },
  "additionalProperties": false
}
```

**Boundaries:**

*This Agent DOES:*
- Analyze code for performance hotspots
- Check real-time safety of processBlock
- Identify memory allocation in audio thread
- Suggest SIMD optimizations
- Recommend data structure improvements
- Profile CPU usage patterns
- Detect lock contention issues

*This Agent DOES NOT:*
- Implement optimizations (use polish-agent)
- Run actual profiling tools (analysis only)
- Modify source files
- Make architectural decisions

**Handoffs:**
- Receives from: polish-agent (during Stage 4 optimization)
- Outputs to: polish-agent (optimization recommendations)
- May invoke: deep-research (for novel optimization patterns)

**Tools:**
- Read (source files)
- Grep (pattern analysis)

---

### cross-plugin-integration-agent

**Priority:** LOW
**Gap Addressed:** No agent specifically handles multi-plugin coordination; module-system skill covers most needs

**Purpose:**
Manages dependencies, shared code, and coordination between multiple plugins. Handles module extraction, version synchronization, and breaking change detection.

**Input Contract:**

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "cross-plugin-integration-agent.input",
  "type": "object",
  "required": ["operation"],
  "properties": {
    "operation": {
      "type": "string",
      "enum": [
        "extract_module",
        "sync_versions",
        "detect_breaking_changes",
        "dependency_graph",
        "impact_analysis"
      ],
      "description": "Type of cross-plugin operation"
    },
    "plugins": {
      "type": "array",
      "items": { "type": "string" },
      "description": "Plugin names involved"
    },
    "module_name": {
      "type": "string",
      "description": "Module name for extraction/analysis"
    },
    "source_plugin": {
      "type": "string",
      "description": "Plugin to extract from"
    },
    "target_plugins": {
      "type": "array",
      "items": { "type": "string" },
      "description": "Plugins to apply changes to"
    }
  },
  "additionalProperties": false
}
```

**Output Contract:**

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "cross-plugin-integration-agent.output",
  "type": "object",
  "required": ["status"],
  "properties": {
    "status": {
      "type": "string",
      "enum": ["success", "error"]
    },
    "dependency_graph": {
      "type": "object",
      "description": "Plugin dependency relationships"
    },
    "breaking_changes": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "change": { "type": "string" },
          "affected_plugins": {
            "type": "array",
            "items": { "type": "string" }
          },
          "migration_path": { "type": "string" }
        }
      }
    },
    "impact_analysis": {
      "type": "object",
      "properties": {
        "plugins_affected": { "type": "integer" },
        "files_changed": { "type": "integer" },
        "risk_level": {
          "type": "string",
          "enum": ["low", "medium", "high"]
        }
      }
    },
    "module_extracted": {
      "type": "object",
      "properties": {
        "name": { "type": "string" },
        "version": { "type": "string" },
        "path": { "type": "string" }
      }
    },
    "error": {
      "type": "object",
      "properties": {
        "code": { "type": "string" },
        "message": { "type": "string" }
      }
    }
  },
  "if": {
    "properties": { "status": { "const": "error" } }
  },
  "then": {
    "required": ["status", "error"]
  },
  "additionalProperties": false
}
```

**Boundaries:**

*This Agent DOES:*
- Extract shared code into modules
- Track module dependencies across plugins
- Detect breaking changes in shared code
- Generate dependency graphs
- Analyze impact of module updates
- Synchronize versions across plugins

*This Agent DOES NOT:*
- Implement code changes (use plugin-improve)
- Make architectural decisions (use plugin-planning)
- Handle single-plugin operations

**Handoffs:**
- Receives from: module-system skill (for complex operations)
- Outputs to: plugin-improve (when changes needed)
- Coordinates with: build-automation (for multi-plugin rebuilds)

**Tools:**
- Read (source files, module manifests)
- Grep (dependency detection)
- Glob (file discovery)

---

## Implementation Priority

| Agent | Priority | Rationale | Phase |
|-------|----------|-----------|-------|
| music-theory-agent | HIGH | Multiple plugins need JI/temperament math; O-IntonationPad, O-Lyrica, future tuning plugins | Future |
| aesthetics-agent | MEDIUM | Improves polish but not blocking; aesthetic-dreaming skill partially addresses | Future |
| performance-profiling-agent | LOW | Would help Stage 4 but current manual approach is functional | Future |
| cross-plugin-integration-agent | LOW | module-system skill covers most needs | Future |

---

## Notes

These specs are ready for implementation but deferred to future phases. Phase 1 focuses on contracts for existing agents.

**Implementation approach:**
1. Create skill directory: `.claude/skills/{agent-name}/`
2. Create SKILL.md with frontmatter and workflow
3. Create BOUNDARIES.md with does/doesn't
4. Create input/output schemas in `.claude/schemas/agent-contracts/`
5. Update CHANGELOG.md with new schema versions
6. Add agent to plugin-registry.json if needed
7. Create reference files for complex workflows

**Integration points to document:**
- Which existing agents will invoke the new agent
- Which stages/phases will use the agent
- What Task tool parameters are needed
- What decision menus appear after agent returns
