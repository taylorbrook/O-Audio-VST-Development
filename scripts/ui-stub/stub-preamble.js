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
// ============================================================================
// stub-preamble.js — installs window.__JUCE__ BEFORE any page script runs.
//
// A classic <script>, not a module, and injected as the first child of <head>
// by serve-ui.js. Modules are deferred; a bundle that reads window.__JUCE__ at
// evaluation time would find nothing if this were one.
//
// Why a preamble is needed at all when generic-juce-stub.js already installs
// the same global: O-TextureForge BUNDLES the JUCE frontend library into
// app.bundle.js and never imports js/juce/index.js, so overlaying that file
// reaches it not at all. The bundled library reads window.__JUCE__ directly and
// throws on a missing initialisationData.
//
// Injected ONLY when the generic stub is in use. The five plugins with their own
// tests/ui-stub/juce-stub.js have committed gates that pass against a tree
// without this file, and altering the page for a plugin whose gate is already
// green is a regression waiting to be misattributed.
//
// TEST FIXTURE ONLY. Never shipped, never embedded.
// ============================================================================

(function () {
  'use strict';

  if (typeof window === 'undefined') return;
  if (window.__JUCE__ && window.__JUCE__.__ouariconStub) return;

  var handlers = new Map();
  var backend = {
    addEventListener: function (name, fn) {
      if (!handlers.has(name)) handlers.set(name, []);
      handlers.get(name).push(fn);
    },
    removeEventListener: function (name, fn) {
      var list = handlers.get(name) || [];
      var i = list.indexOf(fn);
      if (i >= 0) list.splice(i, 1);
    },
    emitEvent: function (name, payload) {
      (handlers.get(name) || []).forEach(function (fn) { try { fn(payload); } catch (e) { console.warn('stub: listener threw for ' + name, e); } });
    },
    // The real backend exposes emitByBackend for C++ -> JS. A browser stub that
    // only offers emitEvent cannot drive the direction the page listens on
    // (pattern_juce_webview_backend_stub_direction), so both spellings are here
    // and both do the same thing.
    emitByBackend: function (name, payload) {
      var v = payload;
      if (typeof v === 'string') { try { v = JSON.parse(v); } catch (e) { /* a plain string is a legitimate payload */ } }
      backend.emitEvent(name, v);
    },
  };

  // The real initialisationData carries the registered surface. The four
  // *includes* arrays are read by the JUCE frontend library before it will hand
  // back a state object; empty arrays are honest (the stub registers nothing
  // through this path) and do not throw, which is the difference that matters.
  window.__JUCE__ = {
    __ouariconStub: true,
    backend: backend,
    initialisationData: {
      __juce__platform: ['ouaricon-stub'],
      __juce__functions: [],
      __juce__sliders: [],
      __juce__toggles: [],
      __juce__comboBoxes: [],
    },
    postMessage: function () {},
    getAndroidUserScripts: function () { return []; },
  };

  window.__stubEmit = function (name, payload) { backend.emitEvent(name, payload); };
})();
