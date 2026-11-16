#pragma once

// Injected with withUserScript: runs at document start, after JUCE's low-level
// integration script (window.__JUCE__.backend exists) and before the page's
// modules. The page itself is the unmodified v1-ui.html.
static const char* kBenchScript = R"JS(
(function () {
  const be = window.__JUCE__.backend;
  const S = { expected: __EXPECTED__, props: new Set(), allPropsDone: false, check: {},
              evals: 0, posts: 0, armed: null, frame: 0, ticking: false, idle: null,
              warns: [], nWarn: 0, nErr: 0, unknownWarn: 0 };
  window.__benchS = S;
  const P = (type, extra) => {
    try { be.emitEvent('__bench', Object.assign({ type: type, pnow: performance.now() }, extra || {})); } catch (e) {}
  };
  P('scriptStart');

  // Console + error capture
  const keep = (kind, args) => {
    const msg = Array.prototype.map.call(args, (a) => { try { return String(a && a.message ? a.message : a); } catch (e) { return '?'; } }).join(' ');
    if (msg.indexOf('unknown to the backend') >= 0) S.unknownWarn++;
    if (S.warns.length < 12) S.warns.push(kind + ': ' + msg.slice(0, 200));
  };
  const ow = console.warn.bind(console), oe = console.error.bind(console);
  console.warn  = function () { S.nWarn++; keep('warn', arguments);  ow.apply(null, arguments); };
  console.error = function () { S.nErr++;  keep('error', arguments); oe.apply(null, arguments); };
  window.addEventListener('error', (e) => { S.nErr++; keep('onerror', [e.message + ' @' + e.filename + ':' + e.lineno]); });
  window.addEventListener('unhandledrejection', (e) => { S.nErr++; keep('rejection', [e.reason]); });

  // Outgoing (page -> C++) message counter
  const origPost = window.__JUCE__.postMessage;
  window.__JUCE__.postMessage = function (s) {
    const m = /"eventId":"([^"]*)"/.exec(s);
    const id = m ? m[1] : '?';
    if (id !== '__bench') {
      S.posts++;
      if (S.idle) S.idle.posts[id] = (S.idle.posts[id] || 0) + 1;
    }
    return origPost.call(window.__JUCE__, s);
  };

  const tick = () => { S.frame++; if (S.ticking) requestAnimationFrame(tick); };
  const startTicker = () => { if (!S.ticking) { S.ticking = true; requestAnimationFrame(tick); } };

  const relayRe = /^__juce__(slider|toggle|comboBox)/;
  const origEmit = be.emitByBackend.bind(be);
  be.emitByBackend = function (eventId, str) {
    if (eventId.indexOf('__bench') === 0) { origEmit(eventId, str); return; }
    S.evals++;
    if (S.idle) S.idle.evals[eventId] = (S.idle.evals[eventId] || 0) + 1;
    const t0 = performance.now();
    origEmit(eventId, str);                  // the page's own listeners run here
    const dt = performance.now() - t0;
    const A = S.armed;
    if (A) { A.dispatchMs += dt; A.bytes += str.length; }

    if (!relayRe.test(eventId)) {
      if (eventId === 'eqSections' && A && A.tCurve === undefined && A.done) {
        A.tCurve = performance.now();
        requestAnimationFrame(() => P('curvePaint', { id: A.id, msArmToCurvePaint: performance.now() - A.t0, framesArmToCurvePaint: S.frame - A.f0 }));
      }
      return;
    }
    if (str.indexOf('"propertiesChanged"') >= 0) {
      if (/(b24|b01)_(freq|type)$/.test(eventId)) {
        const o = JSON.parse(str);
        S.check[eventId.replace(/^__juce__(slider|comboBox)/, '')] =
          o.choices ? { n: o.choices.length, choices: o.choices } : { start: o.start, end: o.end, skew: o.skew, interval: o.interval, name: o.name };
      }
      S.props.add(eventId);
      if (!S.allPropsDone && S.props.size >= S.expected) {
        S.allPropsDone = true;
        P('allProps', { n: S.props.size, evals: S.evals, posts: S.posts, check: S.check });
        requestAnimationFrame(() => {
          P('firstPaint');
          requestAnimationFrame(() => P('secondFrame', { nWarn: S.nWarn, nErr: S.nErr, unknownWarn: S.unknownWarn, warns: S.warns }));
        });
      }
    } else if (A && str.indexOf('"valueChanged"') >= 0) {
      A.ids.add(eventId);
      if (A.tFirst === undefined) { A.tFirst = performance.now(); A.fFirst = S.frame; }
      if (!A.done && A.ids.size >= A.expected) {
        A.done = true;
        const tl = performance.now();
        P('settled', { id: A.id, n: A.ids.size, msArmToFirst: A.tFirst - A.t0, msArmToLast: tl - A.t0,
                       framesArmToFirst: A.fFirst - A.f0, framesArmToLast: S.frame - A.f0,
                       dispatchMs: A.dispatchMs, bytes: A.bytes });
        requestAnimationFrame(() => {
          const tp = performance.now();
          P('settledPaint', { id: A.id, msArmToPaint: tp - A.t0, framesArmToPaint: S.frame - A.f0 });
        });
      }
    }
  };

  be.addEventListener('__bench_arm', (o) => {
    S.armed = { id: o.id, expected: o.expected, ids: new Set(), t0: performance.now(), f0: S.frame,
                done: false, dispatchMs: 0, bytes: 0 };
    startTicker();
  });
  be.addEventListener('__bench_disarm', () => { S.armed = null; S.ticking = false; });

  be.addEventListener('__bench_idle_start', () => {
    S.ticking = false;
    S.idle = { t0: performance.now(), evals: {}, posts: {} };
  });
  be.addEventListener('__bench_idle_end', () => {
    const I = S.idle; S.idle = null;
    if (I) P('idle', { ms: performance.now() - I.t0, evals: I.evals, posts: I.posts });
  });

  // Display frame period: 120 rAF intervals
  be.addEventListener('__bench_raf', () => {
    const ts = [];
    const f = (t) => { ts.push(t); if (ts.length < 121) requestAnimationFrame(f); else {
      const d = []; for (let i = 1; i < ts.length; i++) d.push(ts[i] - ts[i - 1]);
      d.sort((a, b) => a - b);
      P('raf', { median: d[d.length >> 1], mean: (ts[ts.length - 1] - ts[0]) / d.length, max: d[d.length - 1] });
    } };
    requestAnimationFrame(f);
  });

  document.addEventListener('DOMContentLoaded', () => P('dcl', { evals: S.evals, posts: S.posts, props: S.props.size }));
  window.addEventListener('load', () => P('load', { evals: S.evals, posts: S.posts, props: S.props.size }));
})();
)JS";
