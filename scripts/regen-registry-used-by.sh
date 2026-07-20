#!/usr/bin/env bash
#
# regen-registry-used-by.sh — regenerate every module's `used_by` list in
# modules/registry.yaml from DISK TRUTH.
#
# For each module, consumers are derived by grepping plugins/*/CMakeLists.txt
# and plugins/*/Source for the module's own file basenames (the .h/.cpp/.js
# files under its cpp/ and js/ dirs) as fixed strings, plus the module's
# registry name as a CMake `ouaricon_add_module(...)` token. Only the `used_by`
# blocks and the three header lines/comment are
# rewritten; every other line is preserved byte-for-byte. Deterministic and
# idempotent: a second run with no disk changes produces zero further diff.
#
# Usage: bash scripts/regen-registry-used-by.sh   (runs from any cwd)
#
# UPD-02 / IMP-02 / UPD-03 (review 260701-in8).

set -euo pipefail

ROOT="$(git rev-parse --show-toplevel)"

python3 - "$ROOT" <<'PY'
import sys, os, re, subprocess

root = sys.argv[1]
reg_path = os.path.join(root, 'modules', 'registry.yaml')

with open(reg_path, 'r') as f:
    original = f.read()
lines = original.split('\n')
n = len(lines)

# ---------------------------------------------------------------------------
# 1. Locate the modules: section and enumerate module entries.
#    (categories: also uses `  - name:` at 2-space indent — only entries AFTER
#     the top-level `modules:` line count.)
# ---------------------------------------------------------------------------
mod_start = None
for i, ln in enumerate(lines):
    if ln == 'modules:':
        mod_start = i
        break
if mod_start is None:
    print('ERROR: no top-level `modules:` line found', file=sys.stderr)
    sys.exit(1)

entry_starts = []
end_of_modules = n
i = mod_start + 1
while i < n:
    ln = lines[i]
    if re.match(r'^  - name: (\S+)', ln):
        entry_starts.append(i)
    elif re.match(r'^\S', ln):        # column-0 content => modules section ended
        end_of_modules = i
        break
    i += 1

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def module_tokens(path):
    """Fixed-string grep tokens = basenames of *.h/*.cpp/*.js under cpp/ and js/."""
    toks = set()
    for sub in ('cpp', 'js'):
        base = os.path.join(root, 'modules', path, sub)
        for dp, _dirs, files in os.walk(base):
            for fn in files:
                if fn.endswith(('.h', '.cpp', '.js')):
                    toks.add(fn)
    return toks

_ver_cache = {}
def resolve_version(pdir):
    if pdir in _ver_cache:
        return _ver_cache[pdir]
    ver = 'unknown'
    cml = os.path.join(pdir, 'CMakeLists.txt')
    if os.path.exists(cml):
        txt = open(cml, errors='replace').read()
        m = re.search(r'PLUGIN_VERSION\s+"?(\d+\.\d+\.\d+)"?', txt)
        if m:
            ver = m.group(1)
        else:
            for line in txt.split('\n'):
                if 'cmake_minimum_required' in line:
                    continue
                m = re.search(r'VERSION\s+"?(\d+\.\d+\.\d+)"?', line)
                if m:
                    ver = m.group(1)
                    break
    if ver == 'unknown':
        root_cml = os.path.join(root, 'CMakeLists.txt')
        if os.path.exists(root_cml):
            txt = open(root_cml, errors='replace').read()
            m = re.search(r'project\([^)]*VERSION\s+(\d+\.\d+\.\d+)', txt)
            if m:
                ver = m.group(1)
    _ver_cache[pdir] = ver
    return ver

def plugin_is_consumer(pdir, tokens, name):
    cml = os.path.join(pdir, 'CMakeLists.txt')
    src = os.path.join(pdir, 'Source')
    search_paths = [p for p in (cml, src) if os.path.exists(p)]
    # (a) file-basename tokens as fixed strings (skip binaries with -I)
    if tokens and search_paths:
        args = ['grep', '-rlI', '-F']
        for t in sorted(tokens):
            args += ['-e', t]
        args += search_paths
        r = subprocess.run(args, capture_output=True, text=True)
        if r.stdout.strip():
            return True
    # (b) module name as a clean ouaricon_add_module(...) token in CMakeLists.txt
    if os.path.exists(cml):
        pat = r'ouaricon_add_module\([^)]*\b' + re.escape(name) + r'\b'
        r = subprocess.run(['grep', '-rlE', pat, cml], capture_output=True, text=True)
        if r.stdout.strip():
            return True
    return False

# Enumerate candidate consumer plugins once.
plugins_dir = os.path.join(root, 'plugins')
candidates = []
for entry in sorted(os.listdir(plugins_dir)):
    pdir = os.path.join(plugins_dir, entry)
    if os.path.isdir(pdir):
        candidates.append((entry, pdir))

# ---------------------------------------------------------------------------
# 2-5. Per module: derive consumers, build new used_by block, record splice.
# ---------------------------------------------------------------------------
replacements = []   # (ub, be, new_block_lines)
report = []         # (module, old_names, new_names)
any_block_changed = False

for idx, es in enumerate(entry_starts):
    ee = entry_starts[idx + 1] if idx + 1 < len(entry_starts) else end_of_modules
    name = re.match(r'^  - name: (\S+)', lines[es]).group(1)
    path = None
    ub = None
    for j in range(es, ee):
        m = re.match(r'^    path: (\S+)', lines[j])
        if m:
            path = m.group(1)
        if re.match(r'^    used_by:', lines[j]):
            ub = j
    if ub is None or path is None:
        continue

    # used_by block extent: the used_by: line + following 6+-space-indented lines
    be = ub + 1
    while be < ee and re.match(r'^      ', lines[be]):
        be += 1

    old_block = lines[ub:be]
    old_names = [re.match(r'^      - plugin: (\S+)', l).group(1)
                 for l in old_block if re.match(r'^      - plugin: (\S+)', l)]

    tokens = module_tokens(path)
    consumers = []
    for pname, pdir in candidates:
        if plugin_is_consumer(pdir, tokens, name):
            consumers.append(pname)
    consumers.sort(key=str.lower)

    if consumers:
        new_block = ['    used_by:']
        for c in consumers:
            new_block.append(f'      - plugin: {c}')
            new_block.append(f'        version: {resolve_version(os.path.join(plugins_dir, c))}')
    else:
        new_block = ['    used_by: []']

    if new_block != old_block:
        any_block_changed = True
    replacements.append((ub, be, new_block))
    report.append((name, old_names, consumers))

# Apply splices bottom-up so earlier indices stay valid.
for ub, be, new_block in sorted(replacements, key=lambda r: r[0], reverse=True):
    lines[ub:be] = new_block

# ---------------------------------------------------------------------------
# 6. Header handling (UPD-03): reminder comment + version/last_updated bump.
# ---------------------------------------------------------------------------
SENTINEL = 'MUST be bumped on every edit'
COMMENT = ('# NOTE: `version` and `last_updated` ' + SENTINEL +
           '. scripts/regen-registry-used-by.sh bumps them automatically when '
           'used_by content changes; bump minor/major by hand for significant changes.')

vidx = next(i for i, l in enumerate(lines) if re.match(r'^version:', l))
comment_inserted = False
if not (vidx > 0 and SENTINEL in lines[vidx - 1]):
    lines.insert(vidx, COMMENT)
    vidx += 1
    comment_inserted = True

changed = any_block_changed or comment_inserted
if changed:
    today = subprocess.run(['date', '+%Y-%m-%d'], capture_output=True, text=True).stdout.strip()
    for i, l in enumerate(lines):
        if re.match(r'^last_updated:', l):
            lines[i] = f'last_updated: {today}'
            break
    m = re.match(r'^version:\s*(\d+)\.(\d+)\.(\d+)', lines[vidx])
    maj, minr, pat = int(m.group(1)), int(m.group(2)), int(m.group(3))
    lines[vidx] = f'version: {maj}.{minr}.{pat + 1}'

# ---------------------------------------------------------------------------
# 7. Write back only on real change; always emit per-module report.
# ---------------------------------------------------------------------------
assembled = '\n'.join(lines)
if assembled != original:
    with open(reg_path, 'w') as f:
        f.write(assembled)

def fmt(names):
    return '[' + ', '.join(names) + ']' if names else '[]'

for name, old_names, new_names in report:
    print(f'{name}: {fmt(old_names)} -> {fmt(new_names)}')

print('REGEN: changed' if assembled != original else 'REGEN: no-op (already fresh)')
sys.exit(0)
PY
