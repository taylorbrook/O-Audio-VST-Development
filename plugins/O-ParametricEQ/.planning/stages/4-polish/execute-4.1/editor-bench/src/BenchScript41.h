#pragma once

// PLAN-4.1 T26: bench helpers for the 4.1 page rows (E-del-listen,
// E-listen33-aria, E-tipfade, E-tip-lang, E-tip-v13). Injected after
// kBenchScript, kBenchScript32 and kBenchScript33 (same document-start user
// script). The page is the real index.html, unmodified.
static const char* kBenchScript41 = R"JS(
(function () {
  const BENCH_ID = -987654321;
  const P = (type, extra) => {
    try {
      const o = Object.assign({ type: type, pnow: performance.now() }, extra || {});
      window.__benchRawPost(JSON.stringify({ eventId: '__juce__invoke',
        payload: { name: 'benchReport', params: [o], resultId: BENCH_ID } }));
    } catch (e) {}
  };
  const raf = () => new Promise((r) => requestAnimationFrame(r));
  const raf2 = () => new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)));
  const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

  // Every setListenBand post the page makes, timed (wraps kBenchScript's wrapper).
  const T = { listenPosts: [] };
  window.__benchT = T;
  const prevPost = window.__JUCE__.postMessage;
  window.__JUCE__.postMessage = function (s) {
    if (typeof s === 'string' && s.indexOf('"setListenBand"') >= 0) {
      let args = null;
      try { args = JSON.parse(s).payload.params; } catch (e) {}
      T.listenPosts.push({ t: performance.now(), args });
    }
    return prevPost.apply(this, arguments);
  };

  const curve = () => document.getElementById('curve');
  const tipEl = () => document.getElementById('tooltip');
  const ptr = (type, x, y) => {
    const r = curve().getBoundingClientRect();
    curve().dispatchEvent(new PointerEvent(type, { bubbles: true, cancelable: true, composed: true, button: 0,
      buttons: type === 'pointerup' ? 0 : 1, pointerId: 9, pointerType: 'mouse', isPrimary: true,
      clientX: r.left + x, clientY: r.top + y }));
  };
  const clickPlot = (x, y) => { ptr('pointerdown', x, y); ptr('pointerup', x, y); };
  const pressElsewhere = () => {
    const s = document.getElementById('strip') || document.body;
    s.dispatchEvent(new PointerEvent('pointerdown', { bubbles: true, cancelable: true, button: 0, pointerId: 8 }));
    s.dispatchEvent(new PointerEvent('pointerup', { bubbles: true, cancelable: true, button: 0, pointerId: 8 }));
  };
  const tipState = () => {
    const t = tipEl(), cs = getComputedStyle(t), r = t.getBoundingClientRect();
    return { fs: cs.fontSize, op: parseFloat(cs.opacity), point: t.classList.contains('tip-point'),
             visible: t.classList.contains('visible'), i18n: t.dataset.i18n || null, text: t.textContent,
             l: r.left, t: r.top, w: r.width, h: r.height, r: r.right, b: r.bottom,
             vw: window.innerWidth, vh: window.innerHeight, lang: document.documentElement.lang };
  };
  // Per-frame sampler: fn() each rAF until stopped.
  const sampler = (fn) => {
    const s = { on: true, rows: [] };
    const tick = () => { if (!s.on) return; s.rows.push(fn()); requestAnimationFrame(tick); };
    requestAnimationFrame(tick);
    return s;
  };

  Object.assign(window.__bench, {
    // E-del-listen: Delete key on the selected (latched) band. 'delStart' is posted
    // first on the same channel, so C++ reads the native's arrival against it.
    delListen: async (tag) => {
      const n0 = T.listenPosts.length;
      P('delStart', { id: tag });
      const t0 = performance.now();
      document.body.dispatchEvent(new KeyboardEvent('keydown', { key: 'Delete', bubbles: true, cancelable: true }));
      const tSync = performance.now() - t0;
      while (performance.now() - t0 < 400) {
        if (T.listenPosts.slice(n0).some((p) => p.args && p.args[0] === 0)) break;
        await sleep(1);
      }
      const posts = T.listenPosts.slice(n0).map((p) => ({ dt: p.t - t0, args: p.args }));
      const off = posts.find((p) => p.args && p.args[0] === 0);
      P('delListen', { id: tag, posts, dtOff: off ? off.dt : -1, dispatchMs: tSync });
    },

    // E-listen33-aria: #btn-listen aria-pressed and the strip's band, every frame.
    ariaStart: async (tag) => {
      const b = document.getElementById('btn-listen');
      T.aria = sampler(() => ({ a: b.getAttribute('aria-pressed'), loz: (document.getElementById('strip-loz') || {}).textContent || null }));
      await raf();
      P('done', { id: tag, a0: b.getAttribute('aria-pressed') });
    },
    ariaStop: async (tag) => {
      const s = T.aria; s.on = false; await raf();
      const vals = s.rows.map((r) => r.a);
      P('aria', { id: tag, n: vals.length, nTrue: vals.filter((v) => v === 'true').length,
                  nFalse: vals.filter((v) => v === 'false').length, distinct: Array.from(new Set(vals)) });
    },

    // E-tipfade. mode 'fade': show, dismiss, sample every frame for 350 ms.
    // mode 'reshow': show, dismiss, re-show after `at` ms (mid-fade), read 350 ms later.
    // mode 'notrans': inline transition:none (the reduced-motion path: no transitionend),
    // show, dismiss, read at 250 ms (> the fade + 50 ms fallback timer).
    tipFade: async (tag, x, y, mode, at) => {
      const t = tipEl();
      if (mode === 'notrans') t.style.transition = 'none';
      clickPlot(x, y);
      await sleep(200);
      const shown = tipState();
      const t0 = performance.now();
      const s = sampler(() => Object.assign({ dt: performance.now() - t0 }, (({ fs, op, point, visible }) => ({ fs, op, point, visible }))(tipState())));
      pressElsewhere();
      const afterDismiss = tipState();          // synchronously after hideTooltip
      let reshown = null, later = null;
      if (mode === 'reshow') {
        await sleep(at);
        const mid = tipState();
        clickPlot(x, y);
        reshown = Object.assign({ mid }, tipState());
        await sleep(350);
        later = tipState();
        pressElsewhere();
        await sleep(350);
      } else {
        await sleep(mode === 'notrans' ? 250 : 350);
        later = tipState();
      }
      s.on = false;
      if (mode === 'notrans') t.style.transition = '';
      await raf2();
      P('tipFade', { id: tag, mode, shown, afterDismiss, reshown, later, samples: s.rows, end: tipState() });
    },

    // E-tip-lang: show the tip at (x, y), read it, switch language, read it again.
    tipLang: async (tag, x, y, lang) => {
      clickPlot(x, y);
      await sleep(200);
      const before = tipState();
      window.__setLanguage(lang);
      await raf2(); await sleep(40);
      const after = tipState();
      window.__setLanguage('en');
      await raf2();
      pressElsewhere();
      await sleep(250);
      P('tipLang', { id: tag, before, after });
    },

    // E-tip-v13: point tip up, then hover a control (mouseover; the page's 350 ms dwell).
    tipV13: async (tag, x, y, sel) => {
      const tips = document.getElementById('tips-toggle');
      const tipsOn = tips ? tips.getAttribute('aria-pressed') : null;
      clickPlot(x, y);
      await sleep(150);
      const point = tipState();
      const el = document.querySelector(sel);
      if (el) el.dispatchEvent(new MouseEvent('mouseover', { bubbles: true, cancelable: true, relatedTarget: null }));
      await sleep(350 + 200);
      const hover = tipState();
      const want = el ? { title: el.getAttribute('data-tip-title'), body: el.getAttribute('data-tip') } : null;
      window.__setLanguage(document.documentElement.lang || 'en');    // a re-apply while the hover tip shows
      await raf2();
      const reapplied = tipState();
      if (el) el.dispatchEvent(new MouseEvent('mouseout', { bubbles: true, cancelable: true, relatedTarget: document.body }));
      await sleep(300);
      P('tipV13', { id: tag, tipsOn, found: !!el, sel, want, point, hover, reapplied });
    },
  });
})();
)JS";
