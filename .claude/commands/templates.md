---
description: Browse and search the template library for reusable patterns
---

# Template Library Browser

Browse the library of reusable code snippets and prose patterns for JUCE plugin development.

## Usage

```
/templates                    # Show all categories
/templates list               # List all templates
/templates search <query>     # Search by name or tag
/templates show <name>        # Show full template content
/templates stage <1|2|3>      # Show templates for a specific stage
```

## Implementation

### Default (show categories)

```bash
echo "━━━ Template Library ━━━"
echo ""
echo "Categories:"
echo ""
echo "CODE SNIPPETS"
echo "  parameter-binding  - WebView relay and attachment patterns"
echo "  cmake              - CMakeLists.txt configuration"
echo "  webview            - WebView initialization and resources"
echo "  dsp                - Small DSP code patterns"
echo ""
echo "PROSE PATTERNS"
echo "  dsp                - DSP algorithm concepts"
echo "  architecture       - Code organization and threading"
echo "  ui                 - UI behavior patterns"
echo ""
echo "Commands:"
echo "  /templates list              - Show all templates"
echo "  /templates search <query>    - Search by name/tag"
echo "  /templates show <name>       - View full template"
echo "  /templates stage 2           - Templates for Stage 2"
```

### List all templates

```bash
echo "━━━ All Templates ━━━"
echo ""
echo "CODE SNIPPETS:"
for f in .claude/templates/code-snippets/**/*.yaml; do
    name=$(grep "^name:" "$f" | head -1 | cut -d: -f2- | xargs)
    stage=$(grep "^stage:" "$f" | head -1 | cut -d: -f2- | xargs)
    echo "  [$stage] $name"
done

echo ""
echo "PROSE PATTERNS:"
for f in .claude/templates/prose-patterns/**/*.yaml; do
    name=$(grep "^name:" "$f" | head -1 | cut -d: -f2- | xargs)
    stage=$(grep "^stage:" "$f" | head -1 | cut -d: -f2- | xargs)
    complexity=$(grep "^complexity:" "$f" | head -1 | cut -d: -f2- | xargs)
    echo "  [$stage] $name (complexity: $complexity)"
done
```

### Search templates

Search by name or tags in template files:

```bash
QUERY="$1"
echo "━━━ Search: $QUERY ━━━"
echo ""
grep -l -i "$QUERY" .claude/templates/**/*.yaml 2>/dev/null | while read f; do
    name=$(grep "^name:" "$f" | head -1 | cut -d: -f2- | xargs)
    rel_path=${f#.claude/templates/}
    echo "  $name"
    echo "    Path: $rel_path"
done
```

### Show template

Display the full content of a specific template:

```bash
TEMPLATE_NAME="$1"

# Find template by name
FILE=$(grep -l "^name: $TEMPLATE_NAME" .claude/templates/**/*.yaml 2>/dev/null | head -1)

if [ -z "$FILE" ]; then
    # Try partial match
    FILE=$(grep -l -i "name:.*$TEMPLATE_NAME" .claude/templates/**/*.yaml 2>/dev/null | head -1)
fi

if [ -n "$FILE" ]; then
    cat "$FILE"
else
    echo "Template not found: $TEMPLATE_NAME"
    echo ""
    echo "Available templates:"
    grep -h "^name:" .claude/templates/**/*.yaml | cut -d: -f2- | sort | xargs -I{} echo "  - {}"
fi
```

### Filter by stage

Show templates relevant to a specific implementation stage:

```bash
STAGE="$1"
echo "━━━ Stage $STAGE Templates ━━━"
echo ""

for f in .claude/templates/**/*.yaml; do
    file_stage=$(grep "^stage:" "$f" | head -1 | cut -d: -f2- | xargs)
    if [ "$file_stage" = "$STAGE" ] || [ "$file_stage" = "all" ]; then
        name=$(grep "^name:" "$f" | head -1 | cut -d: -f2- | xargs)
        desc=$(grep "^description:" "$f" | head -1 | cut -d: -f2- | xargs | head -c 60)
        echo "  $name"
        echo "    $desc..."
        echo ""
    fi
done
```

## Template Types

### Code Snippets
Exact code with `${variable}` placeholders. Copy and substitute variables.

Example:
```yaml
code:
  header: |
    std::unique_ptr<juce::WebSliderRelay> ${param_name}Relay;
```

### Prose Patterns
Conceptual patterns to interpret and adapt. Read the `concept` section for understanding.

Example:
```yaml
concept: |
  The LFO uses a phase accumulator that wraps at 1.0...
```

## Integration with Workflow

Templates are automatically suggested to subagents based on:
- Current stage (1, 2, or 3)
- Plugin type (effect vs synth)
- Features mentioned in creative brief

Subagents can reference templates in their prompts for consistent implementation.
