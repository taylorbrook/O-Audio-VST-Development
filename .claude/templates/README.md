# Template Library

Reusable patterns for JUCE plugin development, organized into two types:

## Template Types

### Code Snippets (`code-snippets/`)
Exact code blocks with variable substitution. Use when you need:
- Boilerplate that rarely changes (relay setup, CMake patterns)
- Patterns where exact syntax matters
- Copy-paste-ready code

**Format:**
```yaml
code:
  header: |
    std::unique_ptr<juce::WebSliderRelay> ${param_name}Relay;

variables:
  param_name:
    description: "Parameter name in camelCase"
    example: "lfFreq"
```

### Prose Patterns (`prose-patterns/`)
Conceptual descriptions that agents interpret. Use when:
- Implementation varies by context (DSP algorithms)
- Teaching concepts, not just syntax
- Flexibility is more important than exactness

**Format:**
```yaml
concept: |
  The LFO uses a phase accumulator that wraps at 1.0...

implementation_notes:
  - Phase must be instance variable
  - Use AudioParameterChoice for waveform
```

## Directory Structure

```
templates/
├── registry.yaml           # Index of all templates
├── README.md               # This file
├── code-snippets/
│   ├── parameter-binding/  # WebView relay patterns
│   ├── cmake/              # CMakeLists.txt patterns
│   ├── webview/            # WebView initialization
│   └── dsp/                # Small DSP snippets
└── prose-patterns/
    ├── dsp/                # DSP algorithm concepts
    ├── architecture/       # Code organization
    └── ui/                 # UI behavior patterns
```

## Using Templates

### For Subagents

Templates are injected into prompts based on stage and features:

```
Stage 1 (Foundation): cmake/* templates
Stage 2 (DSP): prose-patterns/dsp/*, code-snippets/dsp/*
Stage 3 (GUI): parameter-binding/*, webview/*, ui/*
```

### For Manual Lookup

1. **Browse registry:**
   ```bash
   cat .claude/templates/registry.yaml
   ```

2. **Find by tag:**
   ```bash
   grep -r "tags:.*webview" .claude/templates/
   ```

3. **Read template:**
   ```bash
   cat .claude/templates/code-snippets/parameter-binding/slider-relay.yaml
   ```

## Template Schema

### Code Snippet Required Fields
- `name`: Human-readable name
- `category`: Category from registry
- `stage`: Which implementation stage (1, 2, 3, or "all")
- `tags`: Searchable keywords
- `description`: What the pattern does
- `variables`: Placeholders that need substitution
- `code`: Code blocks with `${variable}` placeholders

### Prose Pattern Required Fields
- `name`: Human-readable name
- `category`: Category from registry
- `stage`: Which implementation stage
- `complexity`: 1 (simple) to 3 (complex)
- `tags`: Searchable keywords
- `description`: Brief summary
- `concept`: Detailed explanation of the pattern
- `implementation_notes`: Practical guidance

### Optional Fields (Both Types)
- `anti_patterns`: Common mistakes to avoid
- `provenance`: Where pattern was learned from
- `related_patterns`: Links to related templates

## Adding New Templates

1. Create YAML file in appropriate directory
2. Follow schema for template type
3. Add entry to `registry.yaml`
4. Test by reading the template

### Code Snippet Example
```yaml
name: My New Pattern
category: webview
stage: 3
tags: [webview, example]
description: Brief description

variables:
  my_var:
    description: What this variable represents
    example: exampleValue

code:
  header: |
    // Code with ${my_var} placeholder
```

### Prose Pattern Example
```yaml
name: My DSP Concept
category: dsp-concepts
stage: 2
complexity: 2
tags: [dsp, example]
description: Brief summary

concept: |
  Detailed explanation of how this works...

implementation_notes:
  - Key point 1
  - Key point 2
```

## Provenance Tracking

Templates track where patterns were learned from:

```yaml
provenance:
  plugins:
    - OuariconAnalogEQ
    - OuariconTremolo
  reference_file: plugins/OuariconAnalogEQ/Source/PluginEditor.cpp
  pattern_id: "stage-3-patterns.md #7"
```

This helps:
- Verify patterns against working code
- Update templates when source plugins change
- Credit the origin of patterns

## Future Enhancements

- [ ] `/templates` command to browse/search interactively
- [ ] Automatic template injection based on creative brief
- [ ] Template versioning and deprecation
- [ ] Template validation against source plugins
