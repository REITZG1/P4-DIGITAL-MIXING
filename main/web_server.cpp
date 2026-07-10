#include "web_server.h"
#include "global_vars.h"

#include <WiFi.h>
#include <esp_log.h>
#include <esp_http_server.h>
#include <cstring>
#include <cstdio>

static const char *TAG = "web";
static httpd_handle_t server = NULL;
static int ws_client_fd = -1;
static int wifiRetryCount = 0;
static unsigned long wifiReconnectDelay = 0;
static unsigned long lastReconnectAttempt = 0;

// --- Embedded web UI ---
static const char* HTML_INDEX = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, viewport-fit=cover, user-scalable=no">
  <meta name="color-scheme" content="dark">
  <meta name="theme-color" content="#0d0e12">
  <title>Digital Mixer ESP32</title>
  <style>
    :root {
      --bg-base: #090a0f;
      --bg-surface: #12141c;
      --panel-bg: linear-gradient(145deg, #161821, #101218);
      --border: rgba(255, 255, 255, 0.05);
      --border-highlight: rgba(255, 255, 255, 0.1);
      --text-main: #f1f3f5;
      --text-dim: #9ca3af;
      --accent-eq: #fb923c;
      --accent-route: #38bdf8;
      --accent-reverb: #c084fc;
      --accent-comp: #34d399;
      --accent-master: #fbbf24;
      --danger: #ef4444;
      --success: #10b981;
      --font-body: 'Inter', -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
      --font-mono: 'JetBrains Mono', "SFMono-Regular", Consolas, monospace;
    }
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; margin: 0; padding: 0; }
    html { scroll-behavior: smooth; }
    body {
      font-family: var(--font-body);
      background-color: var(--bg-base);
      background-image: radial-gradient(circle at 15% 0%, rgba(56,189,248,0.04), transparent 40%), radial-gradient(circle at 85% 100%, rgba(192,132,252,0.04), transparent 40%);
      color: var(--text-main);
      min-height: 100vh;
      font-size: 14px;
      display: flex;
      flex-direction: column;
    }
    ::-webkit-scrollbar { width: 8px; height: 8px; }
    ::-webkit-scrollbar-track { background: var(--bg-base); }
    ::-webkit-scrollbar-thumb { background: rgba(255,255,255,0.1); border-radius: 4px; }
    ::-webkit-scrollbar-thumb:hover { background: rgba(255,255,255,0.2); }

    header {
      position: sticky; top: 0; z-index: 50;
      padding: 12px 20px;
      background: rgba(13, 14, 18, 0.85);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      border-bottom: 1px solid var(--border);
      display: flex; justify-content: space-between; align-items: center; gap: 15px;
      box-shadow: 0 4px 30px rgba(0, 0, 0, 0.5);
    }
    .brand { display: flex; align-items: center; gap: 12px; }
    .brand-icon {
      width: 36px; height: 36px; border-radius: 10px;
      background: linear-gradient(135deg, var(--accent-route), var(--accent-reverb));
      display: flex; align-items: center; justify-content: center;
      box-shadow: 0 0 15px rgba(56,189,248,0.3);
    }
    .brand-icon svg { width: 20px; height: 20px; stroke: #000; }
    .brand-text { line-height: 1.2; }
    .brand-text .title { font-weight: 700; font-size: 1rem; letter-spacing: 0.5px; }
    .brand-text .subtitle {
      font-size: 0.65rem; color: var(--accent-route); letter-spacing: 1px;
      text-transform: uppercase; font-family: var(--font-mono); font-weight: 600;
    }
    .status {
      display: inline-flex; align-items: center; gap: 8px;
      padding: 6px 14px; border-radius: 20px;
      background: rgba(0,0,0,0.4);
      border: 1px solid var(--border);
      font-size: 0.75rem; font-family: var(--font-mono); font-weight: 600;
    }
    .status::before { content: ''; width: 8px; height: 8px; border-radius: 50%; flex-shrink: 0; }
    .status.ws-connected { color: var(--success); border-color: rgba(16,185,129,0.3); }
    .status.ws-connected::before { background: var(--success); box-shadow: 0 0 8px var(--success); animation: pulse 2s infinite; }
    .status.ws-disconnected { color: var(--danger); border-color: rgba(239,68,68,0.3); }
    .status.ws-disconnected::before { background: var(--danger); box-shadow: 0 0 8px var(--danger); }
    @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }

    .channel-nav {
      display: flex; gap: 8px; padding: 12px 20px;
      background: var(--bg-surface);
      border-bottom: 1px solid var(--border);
      overflow-x: auto; scroll-snap-type: x mandatory;
    }
    .ch-pill {
      flex: 0 0 auto; scroll-snap-align: start;
      width: 36px; height: 36px; border-radius: 50%;
      border: 2px solid var(--ch-accent, rgba(255,255,255,0.1));
      background: var(--bg-base); color: var(--text-main);
      font-size: 0.8rem; font-weight: 700; font-family: var(--font-mono);
      cursor: pointer; display: flex; align-items: center; justify-content: center;
      transition: all 0.2s ease;
    }
    .ch-pill:hover, .ch-pill:active { background: var(--ch-accent); color: #000; transform: translateY(-2px); }
    .ch-pill.master-pill {
      width: auto; padding: 0 16px; border-radius: 20px;
      font-size: 0.7rem; letter-spacing: 1px; text-transform: uppercase;
      border-color: var(--accent-master); color: var(--accent-master);
    }
    .ch-pill.master-pill:hover { background: var(--accent-master); color: #000; }

    main {
      padding: 20px;
      display: flex; flex-direction: row; gap: 20px;
      overflow-x: auto; align-items: flex-start;
      scroll-snap-type: x proximity;
      flex-grow: 1;
    }
    #channelsContainer { display: flex; gap: 20px; }

    .panel {
      background: var(--panel-bg);
      border: 1px solid var(--border);
      border-radius: 16px;
      padding: 20px 16px;
      box-shadow: 0 10px 30px -10px rgba(0,0,0,0.8);
      position: relative;
      scroll-snap-align: start;
      display: flex; flex-direction: column;
    }
    #masterPanel {
      min-width: 88vw; max-width: 400px;
      border-color: rgba(251, 191, 36, 0.2);
    }
    .channel-strip {
      min-width: 88vw; max-width: 400px;
      scroll-margin-left: 20px;
    }
    @media (min-width: 768px) {
      main { padding: 24px; gap: 24px; }
      #masterPanel {
        min-width: 300px; max-width: 300px; flex: 0 0 300px;
        position: sticky; left: 24px; z-index: 10;
      }
      .channel-strip {
        min-width: 300px; max-width: 320px;
        scroll-margin-left: 348px;
      }
    }

    .panel h2, .panel h3 { text-align: center; font-weight: 700; margin: 0 0 15px 0; }
    #masterPanel h2 { color: var(--accent-master); letter-spacing: 1px; }
    #masterPanel h3 {
      font-size: 0.75rem; text-transform: uppercase; letter-spacing: 1.5px;
      color: var(--text-dim); margin-top: 24px; padding-top: 12px;
      border-top: 1px dashed var(--border); font-family: var(--font-mono);
    }
    #masterPanel h3:first-of-type { margin-top: 10px; }

    .ch-badge {
      display: inline-flex; align-items: center; justify-content: center;
      width: 28px; height: 28px; border-radius: 8px;
      background: var(--ch-accent, var(--accent-route));
      color: #000; font-weight: 800; font-size: 0.8rem; font-family: var(--font-mono);
      margin-right: 10px;
    }
    .channel-strip h2 { display: flex; align-items: center; justify-content: center; font-size: 1.1rem; }
    .channel-strip::before {
      content: ''; position: absolute; top: 0; left: 0; right: 0; height: 4px;
      background: var(--ch-accent, var(--accent-route)); border-radius: 16px 16px 0 0;
      box-shadow: 0 0 15px var(--ch-accent); opacity: 0.8;
    }

    .slider-row {
      display: flex; align-items: center; justify-content: space-between;
      margin: 10px 0; gap: 12px;
      background: rgba(0,0,0,0.2); padding: 8px 12px; border-radius: 10px;
      border: 1px solid var(--border);
    }
    .slider-row label { flex: 0 0 75px; font-size: 0.75rem; font-weight: 500; color: var(--text-dim); }
    .slider-row span.value {
      width: 45px; text-align: center; font-size: 0.75rem; font-family: var(--font-mono);
      background: #000; color: var(--text-main); border-radius: 6px; padding: 4px;
      flex: 0 0 auto; box-shadow: inset 0 2px 4px rgba(0,0,0,0.5);
    }
    input[type=range] {
      -webkit-appearance: none; appearance: none; flex: 1; height: 8px;
      border-radius: 4px; background: #000; outline: none; cursor: pointer;
      box-shadow: inset 0 1px 3px rgba(0,0,0,0.8);
      --thumb: var(--accent-route);
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none; width: 22px; height: 26px; border-radius: 6px;
      background: var(--bg-surface);
      border: 2px solid var(--thumb);
      box-shadow: 0 2px 5px rgba(0,0,0,0.6); cursor: grab; transition: transform 0.1s;
    }
    input[type=range]::-webkit-slider-thumb:active { cursor: grabbing; transform: scale(1.1); background: var(--thumb); }
    input[type=range]::-moz-range-thumb {
      width: 22px; height: 26px; border-radius: 6px;
      background: var(--bg-surface);
      border: 2px solid var(--thumb); box-shadow: 0 2px 5px rgba(0,0,0,0.6); cursor: grab;
    }
    input[type=range]::-moz-range-thumb:active { cursor: grabbing; background: var(--thumb); }
    input[type=range]::-moz-range-track { height: 8px; border-radius: 4px; background: #000; }

    .master-row input[type=range] { --thumb: var(--accent-master); }
    .eq-row input[type=range]     { --thumb: var(--accent-eq); }
    .route-row input[type=range]  { --thumb: var(--accent-route); }
    .reverb-row input[type=range] { --thumb: var(--accent-reverb); }
    .comp-row input[type=range]   { --thumb: var(--accent-comp); }

    details.ctrl-section { margin-top: 15px; background: rgba(0,0,0,0.15); border-radius: 12px; overflow: hidden; border: 1px solid var(--border); }
    details.ctrl-section > summary {
      list-style: none; cursor: pointer; display: flex; align-items: center; gap: 10px;
      font-size: 0.8rem; font-weight: 600; color: var(--text-main);
      padding: 12px 14px; user-select: none; transition: background 0.2s;
    }
    details.ctrl-section > summary::-webkit-details-marker { display: none; }
    details.ctrl-section > summary:hover { background: rgba(255,255,255,0.05); }
    details.ctrl-section > summary .chev {
      margin-left: auto; width: 8px; height: 8px;
      border-right: 2px solid var(--text-dim); border-bottom: 2px solid var(--text-dim);
      transform: rotate(-45deg); transition: transform 0.3s ease;
    }
    details[open].ctrl-section > summary .chev { transform: rotate(45deg); }
    .section-body { padding: 5px 12px 12px; }

    .vu-container { display: flex; justify-content: center; gap: 20px; margin: 15px 0; }
    .vu-wrapper { display: flex; flex-direction: column; align-items: center; gap: 8px; }
    .vu {
      width: 14px; height: 160px;
      background: #000; border-radius: 4px;
      position: relative; overflow: hidden;
      box-shadow: inset 0 2px 10px rgba(0,0,0,0.8), 0 0 0 1px var(--border);
    }
    .vu-fill {
      position: absolute; bottom: 0; left: 0; width: 100%; height: 0%;
      background: linear-gradient(to top, #10b981 0%, #10b981 55%, #fbbf24 65%, #fbbf24 85%, #ef4444 92%, #ef4444 100%);
      transition: height 0.08s ease-out; z-index: 1;
      box-shadow: 0 0 10px rgba(16, 185, 129, 0.5);
    }
    .vu::after {
      content: ''; position: absolute; inset: 0;
      background-image: repeating-linear-gradient(to bottom, transparent 0, transparent 4px, #000 4px, #000 6px);
      z-index: 2; pointer-events: none;
    }
    .vu-caption { font-size: 0.7rem; color: var(--text-dim); font-family: var(--font-mono); font-weight: 600; }

    .info-box {
      font-size: 0.7rem; line-height: 1.5; color: var(--text-dim);
      background: rgba(251, 191, 36, 0.08); padding: 12px; border-radius: 10px;
      border: 1px solid rgba(251, 191, 36, 0.2); margin-top: 20px; text-align: center;
    }
  </style>
</head>
<body>
<header>
  <div class="brand">
    <div class="brand-icon">
      <svg viewBox="0 0 24 24" fill="none" stroke-width="2.5" stroke-linecap="round">
        <line x1="4" y1="21" x2="4" y2="14"/><line x1="4" y1="10" x2="4" y2="3"/><circle cx="4" cy="12" r="2.5" fill="currentColor" stroke="none"/>
        <line x1="12" y1="21" x2="12" y2="12"/><line x1="12" y1="8" x2="12" y2="3"/><circle cx="12" cy="10" r="2.5" fill="currentColor" stroke="none"/>
        <line x1="20" y1="21" x2="20" y2="16"/><line x1="20" y1="12" x2="20" y2="3"/><circle cx="20" cy="14" r="2.5" fill="currentColor" stroke="none"/>
      </svg>
    </div>
    <div class="brand-text">
      <div class="title">ESP32 DSP Console</div>
      <div class="subtitle">8 Channel Digital Mixer</div>
    </div>
  </div>
  <div class="status" id="wsStatus">WS: Wait...</div>
</header>

<nav class="channel-nav" id="channelNav" aria-label="Navigasi channel"></nav>

<main>
  <div class="panel" id="masterPanel">
    <h2>MASTER BUS</h2>
    <div class="vu-container">
      <div class="vu-wrapper">
        <div class="vu" data-vu="masterL"><div class="vu-fill"></div></div>
        <div class="vu-caption">MAIN L</div>
      </div>
      <div class="vu-wrapper">
        <div class="vu" data-vu="masterR"><div class="vu-fill"></div></div>
        <div class="vu-caption">MAIN R</div>
      </div>
    </div>
    <h3>Master Out L/R</h3>
    <div class="slider-row master-row">
      <label>Level L</label>
      <input type="range" min="0" max="1" step="0.01" data-master="lrL">
      <span class="value">0.80</span>
    </div>
    <div class="slider-row master-row">
      <label>Level R</label>
      <input type="range" min="0" max="1" step="0.01" data-master="lrR">
      <span class="value">0.80</span>
    </div>
    <h3>Sub Groups</h3>
    <div class="slider-row master-row">
      <label>Group 1</label>
      <input type="range" min="0" max="1" step="0.01" data-master="group1">
      <span class="value">0.80</span>
    </div>
    <div class="slider-row master-row">
      <label>Group 2</label>
      <input type="range" min="0" max="1" step="0.01" data-master="group2">
      <span class="value">0.80</span>
    </div>
    <h3>Auxiliary Sends</h3>
    <div class="slider-row master-row">
      <label>Aux 1</label>
      <input type="range" min="0" max="1" step="0.01" data-master="aux1">
      <span class="value">0.80</span>
    </div>
    <div class="slider-row master-row">
      <label>Aux 2</label>
      <input type="range" min="0" max="1" step="0.01" data-master="aux2">
      <span class="value">0.80</span>
    </div>
    <h3>Bus Meters</h3>
    <div class="vu-container" style="gap:15px; margin-top:10px;">
      <div class="vu-wrapper"><div class="vu" style="height:100px;" data-vu="group1"><div class="vu-fill"></div></div><div class="vu-caption">GRP1</div></div>
      <div class="vu-wrapper"><div class="vu" style="height:100px;" data-vu="group2"><div class="vu-fill"></div></div><div class="vu-caption">GRP2</div></div>
      <div class="vu-wrapper"><div class="vu" style="height:100px;" data-vu="aux1"><div class="vu-fill"></div></div><div class="vu-caption">AUX1</div></div>
      <div class="vu-wrapper"><div class="vu" style="height:100px;" data-vu="aux2"><div class="vu-fill"></div></div><div class="vu-caption">AUX2</div></div>
    </div>
    <div class="info-box">
      <b>Info Routing:</b> Slider UI mengontrol pemrosesan DSP internal di ESP. Jalur audio analog dikonfigurasi melalui PCM1808/PCM5102.
    </div>
    </div>
  </div>

  <div id="channelsContainer"></div>
</main>

<script>
  const NUM_CHANNELS = 8;
  const NUM_EQ_BANDS = 15;

  const state = {
    master: { lrL: 0.8, lrR: 0.8, group1: 0.8, group2: 0.8, aux1: 0.8, aux2: 0.8 },
    channels: Array.from({ length: NUM_CHANNELS }, () => ({
      eq: new Array(NUM_EQ_BANDS).fill(0.0),
      send: { lr: 1.0, group1: 0.0, group2: 0.0, aux1: 0.0, aux2: 0.0 },
      reverb: 0.0,
      comp: { ratio: 2.0, attack: 10.0, release: 200.0, threshold: -20.0, gain: 0.0 }
    }))
  };

  const channelsContainer = document.getElementById('channelsContainer');
  const channelNav = document.getElementById('channelNav');

  function createSliderRow(labelText, min, max, step, initValue, onChange, rowClass) {
    const row = document.createElement('div');
    row.className = 'slider-row ' + (rowClass || '');
    const label = document.createElement('label'); label.textContent = labelText;
    const input = document.createElement('input');
    input.type = 'range'; input.min = min; input.max = max; input.step = step; input.value = initValue;
    const span = document.createElement('span'); span.className = 'value'; span.textContent = initValue.toFixed(2);
    row.append(label, input, span);
    input.addEventListener('input', () => {
      const v = parseFloat(input.value);
      span.textContent = (max - min > 2) ? v.toFixed(0) : v.toFixed(2);
      onChange(v);
      scheduleSend();
    });
    return row;
  }

  function createSection(titleText, sectionClass, isOpen) {
    const details = document.createElement('details');
    details.className = 'ctrl-section ' + sectionClass;
    if (isOpen) details.open = true;
    const summary = document.createElement('summary');
    summary.innerHTML = '<span>' + titleText + '</span><span class="chev"></span>';
    const body = document.createElement('div'); body.className = 'section-body';
    details.append(summary, body);
    return { details, body };
  }

  function createChannelStrip(chIndex) {
    const panel = document.createElement('div');
    panel.className = 'panel channel-strip';
    panel.id = 'channel-' + chIndex;
    const hue = Math.round((360 / NUM_CHANNELS) * chIndex);
    panel.style.setProperty('--ch-accent', 'hsl(' + hue + ', 85%, 60%)');

    const title = document.createElement('h2');
    title.innerHTML = '<span class="ch-badge">' + (chIndex + 1) + '</span> CH ' + (chIndex + 1);
    panel.appendChild(title);

    const vuContainer = document.createElement('div');
    vuContainer.className = 'vu-container';
    vuContainer.innerHTML = '<div class="vu-wrapper"><div class="vu" data-vu="ch' + chIndex + '"><div class="vu-fill"></div></div><div class="vu-caption">LEVEL</div></div>';
    panel.appendChild(vuContainer);

    const eqSection = createSection('Graphic EQ 15-Band', 'eq-section', false);
    const freqs = [31, 63, 125, 250, 500, 1000, 2000, 4000, 6000, 8000, 10000, 12000, 14000, 16000, 18000];
    freqs.forEach((f, idx) => {
      eqSection.body.appendChild(createSliderRow(
        f < 1000 ? f + ' Hz' : (f/1000) + ' kHz',
        -12, 12, 0.5, 0.0,
        function(val) { state.channels[chIndex].eq[idx] = val; }, 'eq-row'
      ));
    });
    panel.appendChild(eqSection.details);

    const routeSection = createSection('Bus Routing (Sends)', 'route-section', true);
    ['lr', 'group1', 'group2', 'aux1', 'aux2'].forEach(function(bus, idx) {
      var labels = ['Main L/R', 'Group 1', 'Group 2', 'Aux 1', 'Aux 2'];
      routeSection.body.appendChild(createSliderRow('To ' + labels[idx], 0, 1, 0.01, bus==='lr'?1.0:0.0,
        function(v) { state.channels[chIndex].send[bus] = v; }, 'route-row'));
    });
    panel.appendChild(routeSection.details);

    const reverbSection = createSection('Reverb FX', 'reverb-section', false);
    reverbSection.body.appendChild(createSliderRow('Amount', 0, 1, 0.01, 0.0,
      function(v) { state.channels[chIndex].reverb = v; }, 'reverb-row'));
    panel.appendChild(reverbSection.details);

    const compSection = createSection('Dynamics (Comp)', 'comp-section', false);
    var compParams = [
      { lbl: 'Ratio', min: 1, max: 10, step: 0.1, init: 2.0, key: 'ratio' },
      { lbl: 'Attack', min: 0, max: 100, step: 1, init: 10, key: 'attack' },
      { lbl: 'Release', min: 10, max: 1000, step: 10, init: 200, key: 'release' },
      { lbl: 'Thresh', min: -60, max: 0, step: 1, init: -20, key: 'threshold' },
      { lbl: 'Makeup', min: -12, max: 12, step: 0.5, init: 0, key: 'gain' }
    ];
    compParams.forEach(function(p) {
      compSection.body.appendChild(createSliderRow(p.lbl, p.min, p.max, p.step, p.init,
        function(v) { state.channels[chIndex].comp[p.key] = v; }, 'comp-row'));
    });
    panel.appendChild(compSection.details);

    return panel;
  }

  var masterPill = document.createElement('button');
  masterPill.className = 'ch-pill master-pill';
  masterPill.textContent = 'Master';
  masterPill.onclick = function() { document.getElementById('masterPanel').scrollIntoView({ behavior: 'smooth', inline: 'start' }); };
  channelNav.appendChild(masterPill);

  for (var ch = 0; ch < NUM_CHANNELS; ch++) {
    channelsContainer.appendChild(createChannelStrip(ch));
    var pill = document.createElement('button');
    pill.className = 'ch-pill';
    pill.textContent = ch + 1;
    pill.style.setProperty('--ch-accent', 'hsl(' + Math.round(360/NUM_CHANNELS * ch) + ', 85%, 60%)');
    pill.onclick = (function(idx) { return function() { document.getElementById('channel-' + idx).scrollIntoView({ behavior: 'smooth', inline: 'start' }); }; })(ch);
    channelNav.appendChild(pill);
  }

  document.querySelectorAll('#masterPanel input[type=range]').forEach(function(input) {
    var key = input.dataset.master;
    input.value = state.master[key];
    var span = input.nextElementSibling;
    input.addEventListener('input', function() {
      var v = parseFloat(input.value);
      state.master[key] = v;
      span.textContent = v.toFixed(2);
      scheduleSend();
    });
  });

  var ws = null, sendTimer = null;

  function connectWS() {
    var statusEl = document.getElementById('wsStatus');
    ws = new WebSocket('ws://' + location.host + '/ws');
    ws.onopen = function() {
      statusEl.textContent = 'WS: Connected';
      statusEl.className = 'status ws-connected';
      sendParams();
    };
    ws.onclose = function() {
      statusEl.textContent = 'WS: Disconnected';
      statusEl.className = 'status ws-disconnected';
      setTimeout(connectWS, 2000);
    };
    ws.onerror = function() {
      statusEl.textContent = 'WS: Error';
      statusEl.className = 'status ws-disconnected';
    };
    ws.onmessage = function(ev) {
      try {
        var msg = JSON.parse(ev.data);
        if (msg.type === 'vu') updateVuFromMsg(msg);
      } catch (e) {
        console.warn('WS Parsing Error:', e);
      }
    };
  }

  function scheduleSend() {
    if (sendTimer) clearTimeout(sendTimer);
    sendTimer = setTimeout(function() {
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ type: 'set', master: state.master, channels: state.channels }));
      }
    }, 50);
  }

  function sendParams() { scheduleSend(); }

  function updateVuFromMsg(msg) {
    if (msg.channels) {
      msg.channels.forEach(function(val, idx) {
        var vu = document.querySelector('.vu[data-vu="ch' + idx + '"] .vu-fill');
        if (vu) {
          var height = val * 100;
          vu.style.height = Math.min(Math.max(height, 0), 100).toFixed(1) + '%';
          if (height > 90) vu.style.boxShadow = "0 0 15px rgba(239, 68, 68, 0.8)";
          else vu.style.boxShadow = "0 0 10px rgba(16, 185, 129, 0.5)";
        }
      });
    }
    var bus = msg.bus || {};
    ['masterL','masterR','group1','group2','aux1','aux2'].forEach(function(key) {
      var vu = document.querySelector('.vu[data-vu="' + key + '"] .vu-fill');
      if (vu && typeof bus[key] === 'number') {
        vu.style.height = (bus[key] * 100).toFixed(1) + '%';
      }
    });
  }

  connectWS();
</script>
</body></html>
)rawliteral";

static esp_err_t ws_send_text(const char *text) {
  if (ws_client_fd < 0) return ESP_FAIL;
  httpd_ws_frame_t frame = {};
  frame.type = HTTPD_WS_TYPE_TEXT;
  frame.payload = (uint8_t*)text;
  frame.len = strlen(text);
  return httpd_ws_send_frame_async(server, ws_client_fd, &frame);
}

void webSocketBroadcastVu() {
  if (ws_client_fd < 0) return;
  taskENTER_CRITICAL(&wsMux);
  if (!wsStateChanged) { taskEXIT_CRITICAL(&wsMux); return; }
  wsStateChanged = false;
  DynamicJsonDocument doc(2048);
  doc["type"] = "vu";
  JsonArray chArr = doc.createNestedArray("channels");
  for (int ch = 0; ch < NUM_MIX_CH; ch++) chArr.add(channels[ch].vu);
  JsonObject bus = doc.createNestedObject("bus");
  bus["masterL"] = master.vuMasterL;
  bus["masterR"] = master.vuMasterR;
  bus["group1"] = master.vuGroup1;
  bus["group2"] = master.vuGroup2;
  bus["aux1"] = master.vuAux1;
  bus["aux2"] = master.vuAux2;
  taskEXIT_CRITICAL(&wsMux);
  String output;
  serializeJson(doc, output);
  ws_send_text(output.c_str());
}

static esp_err_t ws_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    ws_client_fd = httpd_req_to_sockfd(req);
    ESP_LOGI(TAG, "WS client connected, fd=%d", ws_client_fd);
    return ESP_OK;
  }
  httpd_ws_frame_t frame = {};
  uint8_t buf[4096] = {0};
  frame.payload = buf;
  frame.len = sizeof(buf) - 1;
  esp_err_t ret = httpd_ws_recv_frame(req, &frame, sizeof(buf) - 1);
  if (ret != ESP_OK) {
    if (ret == ESP_ERR_HTTPD_INVALID_REQ) ws_client_fd = -1;
    return ret;
  }
  if (frame.type == HTTPD_WS_TYPE_TEXT && frame.len > 0) {
    buf[frame.len] = 0;
    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, (const char*)buf);
    if (err) return ESP_OK;
    const char *type = doc["type"];
    if (type && strcmp(type, "set") == 0) {
      JsonObject mst = doc["master"];
      if (!mst.isNull()) {
        master.lrL = mst["lrL"] | master.lrL;
        master.lrR = mst["lrR"] | master.lrR;
        master.group1 = mst["group1"] | master.group1;
        master.group2 = mst["group2"] | master.group2;
        master.aux1 = mst["aux1"] | master.aux1;
        master.aux2 = mst["aux2"] | master.aux2;
      }
      JsonArray chArr = doc["channels"];
      if (!chArr.isNull()) {
        for (int ch = 0; ch < min((int)chArr.size(), NUM_MIX_CH); ch++) {
          JsonObject c = chArr[ch];
          if (c.isNull()) continue;
          JsonArray eqArr = c["eq"];
          if (!eqArr.isNull()) {
            for (int b = 0; b < min((int)eqArr.size(), NUM_EQ_BANDS); b++) {
              channels[ch].eq[b] = eqArr[b] | channels[ch].eq[b];
            }
          }
          JsonObject snd = c["send"];
          if (!snd.isNull()) {
            channels[ch].send.lr = snd["lr"] | channels[ch].send.lr;
            channels[ch].send.group1 = snd["group1"] | channels[ch].send.group1;
            channels[ch].send.group2 = snd["group2"] | channels[ch].send.group2;
            channels[ch].send.aux1 = snd["aux1"] | channels[ch].send.aux1;
            channels[ch].send.aux2 = snd["aux2"] | channels[ch].send.aux2;
          }
          channels[ch].reverb = c["reverb"] | channels[ch].reverb;
          JsonObject comp = c["comp"];
          if (!comp.isNull()) {
            channels[ch].comp.ratio = comp["ratio"] | channels[ch].comp.ratio;
            channels[ch].comp.attack = comp["attack"] | channels[ch].comp.attack;
            channels[ch].comp.release = comp["release"] | channels[ch].comp.release;
            channels[ch].comp.threshold = comp["threshold"] | channels[ch].comp.threshold;
            channels[ch].comp.gain = comp["gain"] | channels[ch].comp.gain;
          }
        }
      }
    }
  }
  return ESP_OK;
}

static esp_err_t api_status_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  DynamicJsonDocument doc(1024);
  doc["sr"] = SampleRateFreq;
  doc["ch"] = NUM_MIX_CH;
  doc["ws"] = (ws_client_fd >= 0);
  doc["mute"] = Mute;
  doc["comp"] = Compressor;
  bool sta_connected = (WiFi.status() == WL_CONNECTED);
  doc["sta_connected"] = sta_connected;
  if (sta_connected) {
    doc["sta_ssid"] = WiFi.SSID();
    doc["sta_ip"] = WiFi.localIP().toString();
  }
  String output;
  serializeJson(doc, output);
  httpd_resp_sendstr(req, output.c_str());
  return ESP_OK;
}

static esp_err_t api_wifi_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  char buf[512] = {0};
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0) {
    httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"no data\"}");
    return ESP_OK;
  }
  buf[ret] = 0;
  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, buf);
  if (err) {
    httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"bad json\"}");
    return ESP_OK;
  }
  const char *ssid = doc["ssid"];
  const char *password = doc["password"] | "";
  if (!ssid || strlen(ssid) == 0) {
    httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"ssid required\"}");
    return ESP_OK;
  }
  String content = String(ssid) + "\n" + String(password);
  if (!writeFile("/wifi.txt", content)) {
    httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"write failed\"}");
    return ESP_OK;
  }
  WiFi.begin(ssid, password);
  httpd_resp_sendstr(req, "{\"ok\":true}");
  ESP_LOGI(TAG, "WiFi config saved, connecting to '%s'", ssid);
  return ESP_OK;
}

static esp_err_t api_save_handler(httpd_req_t *req) {
  saveConfig();
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"ok\":true}");
  return ESP_OK;
}

static esp_err_t api_reboot_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"ok\":true}");
  esp_restart();
  return ESP_OK;
}

static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_sendstr(req, HTML_INDEX);
  return ESP_OK;
}

static const httpd_uri_t uri_index = {
  .uri = "/", .method = HTTP_GET, .handler = index_handler
};
static const httpd_uri_t uri_ws = {
  .uri = "/ws", .method = HTTP_GET, .handler = ws_handler, .is_websocket = true
};
static const httpd_uri_t uri_api_status = {
  .uri = "/api/status", .method = HTTP_GET, .handler = api_status_handler
};
static const httpd_uri_t uri_api_wifi = {
  .uri = "/api/wifi", .method = HTTP_POST, .handler = api_wifi_handler
};
static const httpd_uri_t uri_api_save = {
  .uri = "/api/save", .method = HTTP_POST, .handler = api_save_handler
};
static const httpd_uri_t uri_api_reboot = {
  .uri = "/api/reboot", .method = HTTP_POST, .handler = api_reboot_handler
};

esp_err_t startWebServer() {
  if (server) return ESP_OK;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 12;
  config.stack_size = 8192;
  config.lru_purge_enable = true;
  if (httpd_start(&server, &config) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start HTTP server");
    return ESP_FAIL;
  }
  httpd_register_uri_handler(server, &uri_index);
  httpd_register_uri_handler(server, &uri_ws);
  httpd_register_uri_handler(server, &uri_api_status);
  httpd_register_uri_handler(server, &uri_api_wifi);
  httpd_register_uri_handler(server, &uri_api_save);
  httpd_register_uri_handler(server, &uri_api_reboot);
  ESP_LOGI(TAG, "Web server started on port %d", config.server_port);
  return ESP_OK;
}

esp_err_t stopWebServer() {
  if (server) { httpd_stop(server); server = NULL; }
  ws_client_fd = -1;
  return ESP_OK;
}

esp_err_t initWiFi() {
  ESP_LOGI(TAG, "Initializing WiFi (Arduino)...");

  String ssid = readFile("/wifi.txt");
  String pass = "";
  if (ssid.isEmpty()) { ssid = "mixer"; pass = "12345678"; }
  int newline = ssid.indexOf('\n');
  if (newline > 0) {
    pass = ssid.substring(newline + 1); pass.trim();
    ssid = ssid.substring(0, newline); ssid.trim();
  }

  WiFi.mode(WIFI_AP_STA);

  WiFi.softAP("AudioMixer-8ch", "mixer1234");
  ESP_LOGI(TAG, "AP mode: SSID 'AudioMixer-8ch'");

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
      case ARDUINO_EVENT_WIFI_STA_START:
        WiFi.reconnect();
        break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
        ESP_LOGI(TAG, "STA disconnected (reason: %d)", info.wifi_sta_disconnected.reason);
        unsigned long now = millis();
        if (now - lastReconnectAttempt > wifiReconnectDelay) {
          lastReconnectAttempt = now;
          wifiRetryCount++;
          wifiReconnectDelay = min((unsigned long)(wifiRetryCount * 2000), 60000UL);
          WiFi.reconnect();
        }
        break;
      }
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        wifiRetryCount = 0;
        wifiReconnectDelay = 0;
        ESP_LOGI(TAG, "WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
        break;
    }
  });

  if (ssid.length() > 0 && ssid != "mixer") {
    WiFi.begin(ssid.c_str(), pass.c_str());
    ESP_LOGI(TAG, "STA mode: SSID '%s'", ssid.c_str());
  }

  WiFi.setSleep(false);
  ESP_LOGI(TAG, "WiFi init complete");
  return ESP_OK;
}
