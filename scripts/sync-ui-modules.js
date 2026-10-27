#!/usr/bin/env node
/*
   This file is part of the Ouaricon Audio plugin suite.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    sync-ui-modules.js — perform the configure-time module JS copy without CMake.

    ouaricon_add_module() (modules/cmake/OuariconModules.cmake) copies every
    <module>/js/*.js into plugins/<P>/Source/ui/public/modules/ at CMake
    configure time. Those copies are gitignored (.gitignore, "Source/ui/public/
    modules/"), and CI never configures CMake — so on a fresh clone they do not
    exist, and check-i18n [12] fails on every plugin that serves them
    (red on every push since 2026-09-28). CI runs this before check-i18n.

    Mirrors the CMake function exactly:
      - each un-commented `ouaricon_add_module(<target> <module> ...)` in
        plugins/<P>/CMakeLists.txt
      - module dir = first of modules/{core persistence metering tuning
        modulation synthesis effects ui}/<module> holding a module.yaml
        (unknown module → exit 1, as CMake would FATAL_ERROR)
      - every <moduleDir>/js/*.js copied byte-for-byte (configure_file COPYONLY
        overwrites)

    Locally this writes only what a CMake configure would: the gitignored
    copies, plus the few tracked copies that are byte-identical to their module.

    Direct embed of module JS (no copy at all) is deferred to the Phase 3 UI kit
    (ui-hardening-phase0).

    Usage:
      node scripts/sync-ui-modules.js           copy, one line per file written
      node scripts/sync-ui-modules.js --check   write nothing; exit 1 on drift

  ==============================================================================
*/

'use strict';

const fs   = require('fs');
const path = require('path');

const ROOT        = path.resolve(__dirname, '..');
const PLUGINS_DIR = path.join(ROOT, 'plugins');
const MODULES_DIR = path.join(ROOT, 'modules');

// Search order of MODULE_CATEGORIES in OuariconModules.cmake.
const CATEGORIES = ['core', 'persistence', 'metering', 'tuning',
                    'modulation', 'synthesis', 'effects', 'ui'];

const CHECK = process.argv.includes('--check');

function findModuleDir(name) {
    for (const cat of CATEGORIES) {
        const dir = path.join(MODULES_DIR, cat, name);
        if (fs.existsSync(path.join(dir, 'module.yaml'))) return dir;
    }
    return null;
}

function moduleCalls(cmakeText) {
    const code = cmakeText.split('\n').map(l => l.replace(/#.*$/, '')).join('\n');
    const re = /ouaricon_add_module\s*\(\s*(\S+)\s+([^\s)]+)/g;
    const out = [];
    let m;
    while ((m = re.exec(code)) !== null) out.push(m[2]);
    return out;
}

let errors = 0, changed = 0, current = 0;

for (const plugin of fs.readdirSync(PLUGINS_DIR).sort()) {
    const cmake = path.join(PLUGINS_DIR, plugin, 'CMakeLists.txt');
    if (!fs.existsSync(cmake)) continue;

    for (const mod of moduleCalls(fs.readFileSync(cmake, 'utf8'))) {
        const modDir = findModuleDir(mod);
        if (!modDir) {
            console.error(`ERROR ${plugin}: module '${mod}' not found in any category`);
            errors++;
            continue;
        }
        const jsDir = path.join(modDir, 'js');
        if (!fs.existsSync(jsDir)) continue;

        const destDir = path.join(PLUGINS_DIR, plugin, 'Source', 'ui', 'public', 'modules');
        for (const f of fs.readdirSync(jsDir).filter(f => f.endsWith('.js')).sort()) {
            const src  = fs.readFileSync(path.join(jsDir, f));
            const dest = path.join(destDir, f);
            if (fs.existsSync(dest) && src.equals(fs.readFileSync(dest))) { current++; continue; }

            const rel = path.relative(ROOT, dest);
            changed++;
            if (CHECK) { console.log(`DRIFT ${rel}`); continue; }
            fs.mkdirSync(destDir, { recursive: true });
            fs.writeFileSync(dest, src);
            console.log(`copied ${path.relative(ROOT, path.join(jsDir, f))} -> ${rel}`);
        }
    }
}

console.log(`sync-ui-modules: ${changed} ${CHECK ? 'drifted' : 'written'}, ${current} already current`
            + (errors ? `, ${errors} error(s)` : ''));
process.exit(errors || (CHECK && changed) ? 1 : 0);
