#ifndef ES_WEBPAGES_H
#define ES_WEBPAGES_H

#include <Arduino.h>

// ------------------------------------------------------------
// Interface web do ESPhone — funciona em celular e PC.
// Embutida em flash (PROGMEM), sem precisar de SPIFFS.
// ------------------------------------------------------------
const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0f1420">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
<title>ESPhone</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  :root{
    --bg:#e9edf4; --card:#ffffff; --text:#1a2332; --sub:#5d6b80;
    --line:#dde4ee; --accent:#21A0DC; --contrast:#ffffff; --danger:#e5484d;
    --shadow:0 18px 50px rgba(20,40,80,.25);
  }
  [data-theme="dark"]{
    --bg:#0f1420; --card:#1a2332; --text:#e9eef7; --sub:#8b9aaf;
    --line:#26324a; --accent:#3fb8ec; --shadow:0 18px 50px rgba(0,0,0,.55);
  }
  html,body{height:100%}
  body{
    font-family:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
    background:radial-gradient(1200px 800px at 50% -10%,#2a3550,#0b101c 70%);
    color:var(--text); -webkit-tap-highlight-color:transparent;
  }
  .stage{display:flex;align-items:center;justify-content:center;gap:28px;min-height:100vh;padding:22px;overflow:auto}
  .phone{
    width:min(96vw,400px);height:min(94vh,820px);min-height:520px;
    background:var(--bg);border-radius:40px;overflow:hidden;display:flex;flex-direction:column;
    box-shadow:var(--shadow);border:1px solid rgba(255,255,255,.12);position:relative;
  }
  .statusbar{
    display:flex;align-items:center;justify-content:space-between;
    padding:10px 18px 6px;font-size:12.5px;font-weight:600;color:var(--sub);
  }
  .statusbar .right{display:flex;align-items:center;gap:7px}
  .statusbar svg{width:15px;height:15px;display:block;color:var(--sub)}
  .statusbar svg.off{opacity:.25}
  .batt{display:flex;align-items:center;gap:4px;margin-left:2px}
  .apppill{
    display:flex;align-items:center;justify-content:space-between;
    padding:4px 18px 6px;font-size:12px;font-weight:600;color:var(--sub);
  }
  .apppill .dot{width:8px;height:8px;border-radius:50%;background:var(--accent);box-shadow:0 0 8px var(--accent)}
  #view{flex:1;overflow-y:auto;padding:6px 18px 18px;-webkit-overflow-scrolling:touch}
  #view::-webkit-scrollbar{width:0}
  .home{display:flex;flex-direction:column}
  .clock{margin:18px 0 4px}
  .clock .time{font-size:52px;font-weight:800;letter-spacing:-1px;line-height:1}
  .clock .date{color:var(--sub);font-size:14px;margin-top:4px}
  .pulse{display:inline-block;width:100%;cursor:pointer;background:transparent;border:none;text-align:left}
  h3{font-size:13px;text-transform:uppercase;letter-spacing:1px;color:var(--sub);margin:18px 0 10px}
  .grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(92px,1fr));gap:10px}
  .tile{
    background:var(--card);border:1px solid var(--line);border-radius:18px;padding:14px 6px;
    display:flex;flex-direction:column;align-items:center;gap:8px;cursor:pointer;color:var(--text);
    transition:transform .08s,border-color .15s;font-family:inherit;
  }
  .tile:active{transform:scale(.96)}
  .tile .icon{width:46px;height:46px;border-radius:14px;display:grid;place-items:center;background:var(--card)}
  .tile .icon{background:color-mix(in srgb,var(--accent) 16%,transparent)}
  .tile .icon svg{width:22px;height:22px;stroke:var(--accent)}
  .tile .label{font-size:12px;font-weight:600}
  .page h2{font-size:20px;font-weight:800;margin:10px 0 12px}
  .card{background:var(--card);border:1px solid var(--line);border-radius:16px;padding:14px;margin-bottom:12px}
  .card h3{font-size:12px;margin:2px 0 12px;color:var(--accent)}
  .row{display:grid;grid-template-columns:120px 1fr;align-items:center;gap:10px;padding:8px 0;border-top:1px solid var(--line)}
  .row:first-of-type{border-top:none}
  .row label{font-size:13px;color:var(--sub)}
  .row input[type=text],.row input[type=password],.row select{
    width:100%;background:var(--bg);border:1px solid var(--line);color:var(--text);
    border-radius:10px;padding:8px 10px;font-size:14px;font-family:inherit;outline:none;
  }
  .row input:focus,.row select:focus{border-color:var(--accent)}
  .row input[type=color]{width:56px;height:38px;padding:3px;border:1px solid var(--line);border-radius:10px;background:transparent;cursor:pointer}
  .row input[type=range]{accent-color:var(--accent);width:100%}
  .chk{display:flex;align-items:center;justify-content:space-between;padding:10px 0;border-top:1px solid var(--line)}
  .chk span{font-size:13px;color:var(--sub)}
  .switch{position:relative;width:46px;height:26px;flex:0 0 auto}
  .switch input{opacity:0;width:0;height:0}
  .switch .slider{position:absolute;inset:0;background:var(--line);border-radius:26px;transition:.2s;cursor:pointer}
  .switch .slider:before{content:"";position:absolute;width:20px;height:20px;left:3px;top:3px;background:#fff;border-radius:50%;transition:.2s}
  .switch input:checked + .slider{background:var(--accent)}
  .switch input:checked + .slider:before{transform:translateX(20px)}
  .btn{
    display:block;width:100%;padding:11px;border:none;border-radius:12px;background:var(--accent);
    color:var(--contrast);font-size:14px;font-weight:700;cursor:pointer;font-family:inherit;margin-top:10px;
  }
  .btn.outlined{background:transparent;border:1px solid var(--accent);color:var(--accent)}
  .btn.danger{background:var(--danger);color:#fff;margin-top:10px}
  .btn.ghost{background:transparent;border:1px solid var(--danger);color:var(--danger)}
  .hint{font-size:12px;color:var(--sub);margin-top:8px;line-height:1.5}
  .kv{display:grid;grid-template-columns:auto 1fr;gap:6px 14px;font-size:13px;padding:4px 0}
  .kv b{color:var(--sub);font-weight:600}
  .kv span{text-align:right;font-weight:600;overflow-wrap:anywhere}
  textarea{
    width:100%;min-height:180px;background:var(--bg);border:1px solid var(--line);color:var(--text);
    border-radius:12px;padding:10px;font-size:14px;font-family:inherit;resize:vertical;outline:none;
  }
  textarea:focus{border-color:var(--accent)}
  #bottom{
    display:flex;align-items:center;justify-content:space-around;
    padding:8px 10px calc(8px + env(safe-area-inset-bottom));
    background:var(--card);border-top:1px solid var(--line);
  }
  #bottom button{
    background:none;border:none;cursor:pointer;color:var(--sub);display:grid;place-items:center;padding:8px;border-radius:12px;
  }
  #bottom button.active{color:var(--accent)}
  #bottom button svg{width:22px;height:22px}
  #bottom .ok svg{width:24px;height:24px}
  /* barra lateral (somente desktop) */
  .side{width:272px;display:flex;flex-direction:column;gap:16px}
  .cardcp{background:rgba(16,22,36,.72);border:1px solid rgba(255,255,255,.1);border-radius:18px;padding:18px;color:#dbe3f0;backdrop-filter:blur(6px)}
  .cardcp h4{font-size:13px;letter-spacing:1px;text-transform:uppercase;color:#7fd4ff;margin-bottom:6px}
  .cardcp p{font-size:12.5px;color:#93a3bd;line-height:1.55}
  .devpad{display:flex;flex-direction:column;gap:8px;margin-top:12px}
  .devpad .r2{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px}
  .devpad button{
    padding:11px;border-radius:12px;border:1px solid rgba(255,255,255,.15);background:rgba(255,255,255,.06);
    color:#e6edf8;font-weight:700;font-size:15px;cursor:pointer;font-family:inherit;
  }
  .devpad button.acc{background:var(--accent,#21A0DC);border-color:transparent;color:#fff}
  .ledbox{display:flex;align-items:center;gap:10px;margin-top:14px;font-size:13px;color:#dbe3f0}
  .led{width:13px;height:13px;border-radius:50%;background:#333;transition:box-shadow .1s;flex:0 0 auto}
  .led.on{background:#4cdf6d;box-shadow:0 0 12px #4cdf6d}
  .led.slow{background:#ffd24a;box-shadow:0 0 12px #ffd24a;animation:bl 1.6s infinite}
  .led.fast{background:#ff9d3c;box-shadow:0 0 12px #ff9d3c;animation:bl .5s infinite}
  .led.blink{background:#6aa9ff;box-shadow:0 0 12px #6aa9ff;animation:bl .4s infinite}
  .led.off{background:#40506b}
  @keyframes bl{0%,100%{opacity:1}50%{opacity:.15}}
  .toast{
    position:fixed;left:50%;bottom:110px;transform:translateX(-50%) translateY(20px);
    background:#101828;color:#fff;padding:10px 18px;border-radius:30px;font-size:13.5px;font-weight:600;
    opacity:0;pointer-events:none;transition:.25s;z-index:99;box-shadow:0 8px 26px rgba(0,0,0,.35);max-width:90vw;
  }
  .toast.show{opacity:1;transform:translateX(-50%) translateY(0)}
  @media (max-width:780px){
    .stage{padding:0;overflow:hidden}
    .phone{width:100vw;height:100dvh;min-height:100dvh;border-radius:0;border:none}
    .side{display:none}
  }
  @media (max-height:560px){ .clock .time{font-size:38px} .phone{min-height:100dvh} }
  .tabs{display:flex;gap:6px;margin:4px 0 12px;background:var(--card);border:1px solid var(--line);border-radius:12px;padding:4px}
  .tabs button{flex:1;padding:8px 4px;border:none;background:transparent;color:var(--sub);font-weight:700;font-size:12.5px;border-radius:9px;cursor:pointer;font-family:inherit}
  .tabs button.active{background:var(--accent);color:var(--contrast)}
  .list{display:flex;flex-direction:column;gap:8px}
  .item{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:11px 12px;display:flex;align-items:center;gap:10px;cursor:pointer}
  .item:active{transform:scale(.99)}
  .item .nm{font-weight:700;font-size:14px;flex:1;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
  .item .sub{font-size:11.5px;color:var(--sub);font-weight:500;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
  .item .rssi{font-size:12px;font-weight:800;color:var(--accent);white-space:nowrap}
  .savedbadge{font-size:10px;padding:2px 6px;border-radius:20px;background:color-mix(in srgb,var(--accent) 22%,transparent);color:var(--accent);font-weight:700}
  .bars{display:inline-flex;align-items:flex-end;gap:2px;height:15px}
  .bars i{width:3px;background:var(--line);border-radius:2px;display:block}
  .bars i.on{background:var(--accent)}
  .mini{font-size:11px;color:var(--sub);margin:4px 0 0}
  .stats{display:grid;grid-template-columns:repeat(2,1fr);gap:8px}
  .stat{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:10px}
  .stat b{display:block;font-size:20px;font-weight:800}
  .stat span{font-size:10.5px;color:var(--sub);text-transform:uppercase;letter-spacing:.5px}
  .radar{position:relative;width:100%;aspect-ratio:1;max-width:320px;margin:0 auto;border-radius:50%;background:radial-gradient(circle,color-mix(in srgb,var(--accent) 12%,transparent) 0%,transparent 70%);border:1px solid var(--line);overflow:hidden}
  .radar .ring{position:absolute;inset:22%;border:1px solid var(--line);border-radius:50%}
  .radar .ring.r2{inset:44%}
  .radar .cross{position:absolute;left:50%;top:0;bottom:0;width:1px;background:var(--line)}
  .radar .cross.h{left:0;right:0;top:50%;bottom:auto;height:1px}
  .radar .sweep{position:absolute;inset:0;border-radius:50%;background:conic-gradient(from 0deg,transparent 0deg,color-mix(in srgb,var(--accent) 45%,transparent) 60deg,transparent 61deg);animation:spin 3.2s linear infinite}
  @keyframes spin{to{transform:rotate(360deg)}}
  .radar .blip{position:absolute;width:9px;height:9px;margin:-4.5px;border-radius:50%;background:var(--accent);box-shadow:0 0 10px var(--accent);transition:left .6s,top .6s,opacity .6s;opacity:.95}
  .radar .blip.dim{width:6px;height:6px;margin:-3px;opacity:.55}
  .gattbox{background:var(--bg);border:1px solid var(--line);border-radius:12px;padding:10px;margin-top:8px;font-size:12.5px}
  .gattbox .cu{font-weight:700;color:var(--accent);word-break:break-all}
  .gattbox .ch{display:flex;align-items:center;gap:6px;flex-wrap:wrap;margin-top:6px}
  .pill{font-size:10px;padding:2px 6px;border-radius:20px;border:1px solid var(--line);color:var(--sub)}
  .pill.on{background:color-mix(in srgb,var(--accent) 20%,transparent);color:var(--accent);border-color:transparent}
  .tagbtn{font-size:11px;padding:4px 9px;border-radius:9px;border:1px solid var(--accent);background:transparent;color:var(--accent);font-weight:700;cursor:pointer;font-family:inherit}
  .tagbtn.solid{background:var(--accent);color:var(--contrast);border-color:transparent}
  .log{font-family:ui-monospace,Menlo,monospace;font-size:11px;background:var(--bg);border:1px solid var(--line);border-radius:10px;padding:8px;max-height:120px;overflow:auto;color:var(--sub);word-break:break-all;margin-top:6px}
  .warn{background:color-mix(in srgb,var(--danger) 12%,transparent);border-color:color-mix(in srgb,var(--danger) 40%,transparent)}
  .gpio-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:8px}
  .gpio-pin{display:flex;flex-direction:column;align-items:center;padding:8px;background:var(--card);border:1px solid var(--line);border-radius:10px;gap:4px;min-width:70px}
  .gpio-pin.reserved{opacity:0.5;background:color-mix(in srgb,var(--danger) 8%,transparent);border-color:color-mix(in srgb,var(--danger) 30%,transparent)}
  .gpio-pin .pin-num{font-weight:700;font-size:12px;color:var(--accent)}
  .gpio-pin .pin-mode{font-size:10px;color:var(--sub);text-transform:uppercase}
  .gpio-pin .pin-val{font-family:ui-monospace,monospace;font-size:12px;font-weight:700;color:var(--text)}
  .gpio-pin .gpio-toggle{margin-top:6px;font-size:11px;padding:4px 8px}
  .gpio-pin.reserved .gpio-toggle{opacity:0.4;cursor:not-allowed}
</style>
</head>
<body data-theme="dark">
<div class="stage">
  <div class="phone">
    <div class="statusbar">
      <span id="devtime">--:--</span>
      <div class="right">
        <svg id="wifi" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M5 12.5a10 10 0 0 1 14 0"/><path d="M8.5 16a5 5 0 0 1 7 0"/><circle cx="12" cy="19.6" r="1.2" fill="currentColor" stroke="none"/></svg>
        <svg id="bt" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="6.5 6.5 17.5 17.5 12 23 12 1 17.5 6.5 6.5 17.5"/></svg>
        <span class="batt"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="1.5" y="7" width="15" height="10" rx="2.5"/><line x1="20.5" y1="11" x2="20.5" y2="13"/></svg></span>
        <span style="font-size:11px">USB</span>
      </div>
    </div>
    <div class="apppill">
      <span id="appname">Início</span>
      <span class="dot"></span>
    </div>
    <div id="view"></div>
    <div id="bottom">
      <button data-action="nav-home" title="Início"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 10.5 12 3l9 7.5V21a1 1 0 0 1-1 1h-5.5v-7h-3v7H4a1 1 0 0 1-1-1Z"/></svg></button>
      <button data-action="nav-prev" title="App anterior"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="15 18 9 12 15 6"/></svg></button>
      <button class="ok" data-action="nav-select" title="Abrir / OK"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="9"/><circle cx="12" cy="12" r="2.6" fill="currentColor" stroke="none"/></svg></button>
      <button data-action="nav-next" title="Próximo app"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 18 15 12 9 6"/></svg></button>
      <button data-action="nav-back" title="Voltar"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="19" y1="12" x2="5" y2="12"/><polyline points="12 19 5 12 12 5"/></svg></button>
    </div>
  </div>

  <div class="side">
    <div class="cardcp">
      <h4>Painel físico</h4>
      <p>Mesmos comandos do botão <b>BOOT</b> da placa. O <b>LED</b> pisca indicando o app selecionado.</p>
      <div class="devpad">
        <button data-action="nav-home">⌂ Home</button>
        <div class="r2">
          <button data-action="nav-prev">◀</button>
          <button class="acc" data-action="nav-select">OK</button>
          <button data-action="nav-next">▶</button>
        </div>
        <button data-action="nav-back">← Voltar</button>
      </div>
      <div class="ledbox"><span id="leddot" class="led off"></span>LED:&nbsp;<span id="ledtxt">—</span></div>
    </div>
    <div class="cardcp">
      <h4>Como usar</h4>
      <p>• <b>Aperto curto</b> no BOOT = próximo app (LED pisca a posição).<br>
         • <b>Segurar</b> o BOOT = abrir o app.<br>
         • Dentro do app, apertar o BOOT volta para a tela inicial.</p>
    </div>
  </div>
</div>
<div class="toast" id="toast"></div>

<script>
const ICONS=[
 '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><line x1="4" y1="21" x2="4" y2="14"/><line x1="4" y1="10" x2="4" y2="3"/><line x1="12" y1="21" x2="12" y2="12"/><line x1="12" y1="8" x2="12" y2="3"/><line x1="20" y1="21" x2="20" y2="16"/><line x1="20" y1="12" x2="20" y2="3"/><line x1="1" y1="14" x2="7" y2="14"/><line x1="9" y1="8" x2="15" y2="8"/><line x1="17" y1="16" x2="23" y2="16"/></svg>',
 '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="6.5 6.5 17.5 17.5 12 23 12 1 17.5 6.5 6.5 17.5"/></svg>',
 '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M5 12.5a10 10 0 0 1 14 0"/><path d="M8.5 16a5 5 0 0 1 7 0"/><circle cx="12" cy="19.6" r="1.2" fill="currentColor" stroke="none"/><line x1="2" y1="3" x2="22" y2="3"/></svg>',
 '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><circle cx="12" cy="12" r="10"/><line x1="12" y1="16" x2="12" y2="12"/><line x1="12" y1="8.2" x2="12.01" y2="8.2"/></svg>',
 '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>',
 '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M17 3a2.85 2.85 0 0 1 4 4L7.5 20.5 2 22l1.5-5.5Z"/></svg>',
 '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="3" width="20" height="14" rx="2"/><path d="M8 21h8"/><line x1="12" y1="17" x2="12" y2="3"/></svg>'
];

let state=null, cfg=null, viewKey='', sysTimer=null, appTimer=null, lastAppIdx=-1;
const $=id=>document.getElementById(id);
const pad=n=>n<10?'0'+n:''+n;
const fmtClock=d=>pad(d.getHours())+':'+pad(d.getMinutes());
const MONTHS=['janeiro','fevereiro','março','abril','maio','junho','julho','agosto','setembro','outubro','novembro','dezembro'];

async function apiGet(p){try{const r=await fetch('/api/'+p);return await r.json();}catch(e){return null;}}
async function apiPost(p,body){try{const r=await fetch('/api/'+p,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});return await r.json();}catch(e){return null;}}
function toast(m){const t=$('toast');t.textContent=m;t.classList.add('show');setTimeout(()=>t.classList.remove('show'),2200);}

function hexToInt(h){return parseInt(h.slice(1),16)||0;}
function intToHex(n){return '#'+((n>>>16)&255).toString(16).padStart(2,'0')+((n>>>8)&255).toString(16).padStart(2,'0')+(n&255).toString(16).padStart(2,'0');}
function applyTheme(){
  if(!cfg){document.body.dataset.theme='dark';return;}
  const t=cfg.theme==='auto'?(matchMedia('(prefers-color-scheme: light)').matches?'light':'dark'):cfg.theme;
  document.body.dataset.theme=t;
  document.documentElement.style.setProperty('--accent',intToHex(cfg.accent));
}

async function refresh(){
  const s=await apiGet('state');
  if(!s){return;}
  state=s;
  $('devtime').textContent=fmtClock(new Date());
  $('appname').textContent=s.inApp?s.apps[s.appIndex]:'Início';
  $('leddot').className='led '+s.led.pattern;
  $('ledtxt').textContent=s.led.pattern;
  $('bt').classList.toggle('off',!s.ble);
  $('wifi').classList.toggle('off',!s.wifi.sta && !s.wifi.enabled);
  const key=s.inApp+':'+s.appIndex;
  if(key!==viewKey){viewKey=key;renderView();}
  // destaca botão home quando na home
  document.querySelectorAll('#bottom button').forEach(b=>b.classList.toggle('active',b.dataset.action==='nav-home'&&!s.inApp));
}

function renderView(){
  if(!state)return;
  stopAppTimer();
  const newIdx=state.inApp?state.appIndex:-1;
  if(lastAppIdx===1&&newIdx!==1){apiPost('ble/scan',{active:false});}
  lastAppIdx=newIdx;
  if(newIdx>=0){loadApp(newIdx);}
  else{$('view').innerHTML=homeHtml(state);}
}
function pageHtml(title,inner){
  return '<div class="page"><h2>'+title+'</h2>'+inner+'</div>';
}
function homeHtml(s){
  const d=new Date();
  const date=d.getDate()+' de '+MONTHS[d.getMonth()]+' de '+d.getFullYear();
  const grid=s.apps.map((a,i)=>'<button class="tile" data-action="nav-goto" data-index="'+i+'"><span class="icon">'+ICONS[i]+'</span><span class="label">'+a+'</span></button>').join('');
  return '<div class="home"><div class="clock"><div class="time">'+fmtClock(d)+'</div><div class="date">'+date+'</div></div><h3>Aplicativos</h3><div class="grid">'+grid+'</div></div>';
}

function loadApp(i){
  stopSys();
  if(i===0)loadSettings();
  else if(i===1)loadBle();
  else if(i===2)loadWifiMon();
  else if(i===3)loadAbout();
  else if(i===4)loadSystem();
  else if(i===5)loadNotes();
  else if(i===6)loadGpio();
  else if(i===7)loadModulos();
}

async function loadSettings(){
  const key=viewKey;
  $('view').innerHTML='<div class="page"><h2>Configurações</h2><div class="card">Carregando…</div></div>';
  cfg=await apiGet('config');
  if(viewKey!==key||!cfg)return;
  applyTheme();
  $('view').innerHTML=settingsHtml(cfg);
}
async function refreshSettings(){
  const cfg=await apiGet('config');
  if(!cfg)return;
  const st=$('hunterStatus');
  if(st)st.textContent=cfg.wifiHunter?'Ativo':'Parado';
  const cb=$('chkWifiHunter');
  if(cb)cb.checked=cfg.wifiHunter;
  const iv=$('inpHunterInterval');
  if(iv)iv.value=cfg.hunterInterval||30;
}
function settingsHtml(c){
  const accent=intToHex(c.accent);
  const tabsApp=[[0,'WiFi'],[1,'Bluetooth'],[2,'Sistema'],[3,'Dispositivo']];
  $('view').dataset.tab='0';
  setTabBtn=function(t){const b=$('tab-'+t);if(b)b.classList.add('on');};
  window._setTab=setTabBtn;
  return pageHtml('Configurações','<div class="tabsrow"><button class="tab" id="tab-0" data-tab="0" onclick="selTab(0)">WiFi</button><button class="tab" id="tab-1" data-tab="1" onclick="selTab(1)">Bluetooth</button><button class="tab" id="tab-2" data-tab="2" onclick="selTab(2)">Sistema</button><button class="tab" id="tab-3" data-tab="3" onclick="selTab(3)">Dispositivo</button></div>'+
  
// ===== ABA 0: WiFi =====
   '<div class="card" data-tab="0"><h3>WiFi — Access Point</h3>'+
      chk('chkAP','Manter Access Point ligado',c.apEnabled)+
      row('SSID',text('inpApSsid',c.apSsid))+
      row('Senha',pass('inpApPass',c.apPassword))+
      '<div class="row"><label>Compartilhar Internet com AP</label>'+
        '<label class="switch"><input type="checkbox" id="inpApNat"'+(c.apShareInternet?' checked':'')+'><span class="slider"></span></label></div>'+
      '<div class="hint">Ao ligar, clientes conectados neste Access Point usam a internet da conexão STA (rede da sua casa). Requer STA conectado e reinício.</div>'+
    '</div>'+
    '<div class="card" data-tab="0"><h3>WiFi — Cliente (STA)</h3>'+
      '<p class="hint">Conecte o ESPhone à rede da sua casa. A página continua acessível pelo IP abaixo quando conectado e o AP pode compartilhar esta internet.</p>'+
      row('Rede (SSID)',text('inpStaSsid',c.staSsid))+
      row('Senha',pass('inpStaPass',c.staPassword))+
      '<button class="btn outlined" data-action="save-sta">Conectar à rede WiFi</button>'+
    '</div>'+
    '<div class="card" data-tab="0"><h3>Caça-WiFi (Auto-connect livre)</h3>'+
      '<p class="hint">Escaneia redes abertas/sem senha. Ao achar, espera 3s para confirmar que não é passageiro (alta velocidade). Se confirmar, conecta automaticamente.</p>'+
      chk('chkWifiHunter','Ativar Caça-WiFi',c.wifiHunter)+
      row('Intervalo scan (s)',text('inpHunterInterval',c.hunterInterval?String(c.hunterInterval):'30'))+
      '<div class="row" style="align-items:center;justify-content:space-between">'+
        '<span>Status: <b id="hunterStatus">'+(c.wifiHunter?'Ativo':'Parado')+'</b></span>'+
        '<button class="btn" data-action="hunter-scan">Escanear agora</button>'+
      '</div>'+
      '<div class="hint" id="hunterHint">LED Morse: .-.. = achou rede • - - . = aguardando 3s • . . - = conectado • . . = falhou</div>'+
    '</div>'+

  // ===== ABA 1: Bluetooth =====
   '<div class="card" data-tab="1"><h3>Bluetooth — Radar BLE (LED externo 2 pinos)</h3>'+
     '<div class="hint">Radar BLE que acende um LED/alto-falante externo de 2 pinos a cada beep do scan. Ativo ignora a suspensão e sempre reflete o radar nesses pinos.</div>'+
     chk('ledScanExt','Usar alto-falante externo de scan',c.ledScanExt)+
     row('Pino POSITIVO (GPIO)',text('inpScanPos',c.scanSpeakPosPin?String(c.scanSpeakPosPin):''))+
     row('Pino NEGATIVO (GPIO)',text('inpScanNeg',c.scanSpeakNegPin?String(c.scanSpeakNegPin):''))+
   '</div>'+

  // ===== ABA 2: Sistema =====
   '<div class="card" data-tab="2"><h3>Sistema — Testes</h3>'+
     '<button class="btn" data-action="self-test">Self-test LED (embutido + externo)</button>'+
   '</div>'+
   '<div class="card" data-tab="2" style="border-color:var(--danger)"><h3 style="color:var(--danger)">Zona de perigo</h3>'+
     '<button class="btn danger" data-action="reboot">Reiniciar dispositivo</button>'+
     '<button class="btn ghost" data-action="reset">Restaurar configurações de fábrica</button>'+
   '</div>'+

  // ===== ABA 3: Dispositivo =====
   '<div class="card" data-tab="3"><h3>Dispositivo</h3>'+
     row('Nome do dispositivo',text('inpName',c.deviceName))+
     '<div class="row"><label>Tema</label><select id="selTheme">'+opt('auto',c.theme)+opt('light',c.theme)+opt('dark',c.theme)+'</select></div>'+
     '<div class="row"><label>Cor de destaque</label><input type="color" id="inpAccent" value="'+accent+'"></div>'+
     '<div class="row"><label>Brilho do LED</label><div><input type="range" id="rngBri" min="1" max="255" value="'+c.ledBrightness+'"><span id="briVal" style="font-size:12px;color:var(--sub)">'+c.ledBrightness+'</span></div></div>'+
     chk('ledInvert','LED invertido (ativo em baixo)',c.ledInvert)+
   '</div>'+
   '<button class="btn" data-action="save-config">Salvar configurações</button>');
}
function row(label,ctrl){return '<div class="row"><label>'+label+'</label>'+ctrl+'</div>';}
function text(id,v){return '<input type="text" id="'+id+'" value="'+esc(v)+'">';}
function pass(id,v){return '<input type="password" id="'+id+'" value="'+esc(v)+'">';}
function opt(v,cur){return '<option value="'+v+'"'+(cur===v?' selected':'')+'>'+v+'</option>';}
function chk(id,label,on){return '<div class="chk"><span>'+label+'</span><label class="switch"><input type="checkbox" id="'+id+'"'+(on?' checked':'')+'><span class="slider"></span></label></div>';}
function esc(s){return String(s==null?'':s).replace(/&/g,'&amp;').replace(/"/g,'&quot;').replace(/</g,'&lt;');}
window.esc=esc;

async function loadAbout(){
  const key=viewKey;
  $('view').innerHTML='<div class="page"><h2>Sobre</h2><div class="card" data-tab="3">Carregando…</div></div>';
  const i=await apiGet('info');
  if(viewKey!==key||!i)return;
  const d=new Date();
  $('view').innerHTML=pageHtml('Sobre',
   '<div class="card" data-tab="3"><div class="kv">'+
   kv('Dispositivo',i.deviceName)+kv('Firmware','ESPhone v'+i.version)+kv('Data/Hora',d.toLocaleDateString('pt-BR')+' '+fmtClock(d))+
   '</div></div>'+
   '<div class="card" data-tab="3"><h3>Hardware</h3><div class="kv">'+
   kv('Chip',i.chip)+kv('Revisão',i.revision)+kv('Núcleos',i.cores)+kv('Frequência',i.cpuFreq+' MHz')+
   kv('Flash',(i.flashSize/1048576).toFixed(1)+' MB')+kv('PSRAM',i.psram?(i.psramSize/1048576).toFixed(1)+' MB':'—')+
   kv('Temperatura',i.temp.toFixed(1)+' °C')+kv('Memória livre',(i.heapFree/1024).toFixed(0)+' KB')+kv('Uptime',fmtUptime(i.up))+
   '</div></div>'+
   '<div class="card" data-tab="3"><h3>Rede</h3><div class="kv">'+
   kv('Modo WiFi',i.wifiMode)+kv('IP',i.ip)+kv('Rede (STA)',i.staSsid||'—')+kv('Sinal RSSI',i.rssi+' dBm')+
   kv('Clientes AP',i.apClients)+kv('MAC STA',i.macSta)+kv('MAC AP',i.macAp)+kv('Bluetooth',i.ble?'ativo':'desligado')+
   '</div></div>');
}
function kv(k,v){return '<div><b>'+k+'</b></div><div><span>'+esc(v)+'</span></div>';}

async function loadSystem(){
  const key=viewKey;stopSys();
  $('view').innerHTML='<div class="page"><h2>Sistema</h2><div class="card" data-tab="3">Carregando…</div></div>';
  const i=await apiGet('info');
  if(viewKey!==key||!i)return;
  $('view').innerHTML=pageHtml('Sistema',
   '<div class="card" data-tab="3"><div class="kv">'+
   kv('Frequência',id('f_cpu'))+kv('Flash',id('f_flash'))+kv('PSRAM',id('f_psram'))+
   kv('Memória livre',id('f_heap'))+kv('Maior bloco',id('f_maxheap'))+kv('Temperatura',id('f_temp'))+
   kv('Uptime',id('f_up'))+
   '</div></div>'+
   '<div class="card" data-tab="3"><h3>Rede</h3><div class="kv">'+
   kv('Modo',id('f_wifiMode'))+kv('IP',id('f_ip'))+kv('Rede STA',id('f_wifiSsid'))+kv('Sinal',id('f_rssi'))+kv('Clientes AP',id('f_apClients'))+kv('Bluetooth',id('f_ble'))+
   '</div></div>'+
   '<button class="btn danger" data-action="reboot">Reiniciar dispositivo</button>');
  fillInfo(i);
  sysTimer=setInterval(async()=>{
    if(viewKey!==key){stopSys();return;}
    const j=await apiGet('info');
    if(viewKey===key&&j)fillInfo(j);
  },2000);
}
function id(k){return '<span id="'+k+'">—</span>';}
function fillInfo(i){
  const m={'f_cpu':i.cpuFreq+' MHz','f_flash':(i.flashSize/1048576).toFixed(1)+' MB','f_psram':i.psram?(i.psramSize/1048576).toFixed(1)+' MB':'—',
   'f_heap':(i.heapFree/1024).toFixed(0)+' KB','f_maxheap':(i.heapMax/1024).toFixed(0)+' KB','f_temp':i.temp.toFixed(1)+' °C','f_up':fmtUptime(i.up),
   'f_wifiMode':i.wifiMode,'f_ip':i.ip,'f_wifiSsid':i.staSsid||'—','f_rssi':i.rssi+' dBm','f_apClients':i.apClients+' cliente(s)','f_ble':i.ble?'ativo':'desligado'};
  for(const k in m){const el=$(k);if(el)el.textContent=m[k];}
}
function stopSys(){if(sysTimer){clearInterval(sysTimer);sysTimer=null;}}
function fmtUptime(s){
  const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60),x=s%60;
  return (d>0?d+'d ':'')+(h>0?h+'h ':'')+m+'min '+x+'s';
}

async function loadNotes(){
  const key=viewKey;
  $('view').innerHTML='<div class="page"><h2>Notas</h2><div class="card" data-tab="3">Carregando…</div></div>';
  const n=await apiGet('notes');
  if(viewKey!==key)return;
  $('view').innerHTML=pageHtml('Notas',
   '<div class="card" data-tab="3"><textarea id="txtNotes" placeholder="Escreva aqui…">'+esc(n&&n.notes?n.notes:'')+'</textarea>'+
   '<button class="btn" data-action="save-notes">Salvar</button>'+
   '<p class="hint">Máximo de 2000 caracteres. Salvo na memória do ESP32.</p></div>');
}

async function loadGpio(){
  const key=viewKey;
  $('view').innerHTML='<div class="page"><h2>GPIO</h2><div class="card" data-tab="3">Carregando…</div></div>';
  const data=await apiGet('gpio');
  if(viewKey!==key||!data)return;
  renderGpio(data);
  gpioTimer=setInterval(async()=>{
    if(viewKey!==key){clearInterval(gpioTimer);gpioTimer=null;return;}
    const d=await apiGet('gpio');
    if(viewKey===key&&d)updateGpioVals(d);
  },1000);
}
let gpioTimer=null;

function renderGpio(data){
  const reserved=data.reserved||0;
  let html='<div class="card" data-tab="3"><h3>Controle de GPIOs (0-39)</h3>'+
    '<p class="hint">Pinos reservados pelo sistema (LED, botão, UART, LED externo) estão bloqueados.</p></div>';
  
  // Agrupa em 4 colunas de 10 pinos
  for(let col=0;col<4;col++){
    html+='<div class="card" data-tab="3"><div class="gpio-grid">';
    for(let row=0;row<10;row++){
      const pin=col*10+row;
      if(pin>39) break;
      const p=data.pins[pin];
      const isReserved=!!(reserved & (1<<pin));
      const modeHtml=p.mode? 'OUT':'IN';
      const valHtml=p.val===1?'HIGH':'LOW';
      const disabled=isReserved?' disabled':'';
      const resCls=isReserved?' reserved':'';
      html+=
        '<div class="gpio-pin'+resCls+'" data-pin="'+pin+'">'+
          '<div class="pin-num">GPIO'+pin+'</div>'+
          '<div class="pin-mode">'+modeHtml+'</div>'+
          '<div class="pin-val" id="gpio-val-'+pin+'">'+valHtml+'</div>'+
          '<button class="btn gpio-toggle" data-pin="'+pin+'" data-on="'+(p.val===1?0:1)+'"'+disabled+'>'+
            (p.val===1?'Desligar':'Ligar')+
          '</button>'+
        '</div>';
    }
    html+='</div></div>';
  }
  $('view').innerHTML=html;
  
  // Event listeners para botões toggle
  document.querySelectorAll('.gpio-toggle').forEach(btn=>{
    btn.addEventListener('click',async e=>{
      const pin=parseInt(e.target.dataset.pin);
      const on=parseInt(e.target.dataset.on);
      e.target.disabled=true;
      const r=await apiPost('gpio',{pin:pin,on:on===1});
      e.target.disabled=false;
      if(r&&r.ok){
        e.target.dataset.on=on===1?0:1;
        e.target.textContent=on===1?'Desligar':'Ligar';
        const ve=$('gpio-val-'+pin);
        if(ve)ve.textContent=on===1?'HIGH':'LOW';
        toast('GPIO'+pin+' '+(on===1?'ligado':'desligado'));
      }else if(r&&r.error==='reserved'){
        toast('GPIO'+pin+' reservado pelo sistema');
      }else{
        toast('Falha ao alterar GPIO'+pin);
      }
    });
  });
}

function updateGpioVals(data){
  data.pins.forEach(p=>{
    const ve=$('gpio-val-'+p.pin);
    if(ve)ve.textContent=p.val===1?'HIGH':'LOW';
  });
}

function stopAppTimer(){if(appTimer){clearInterval(appTimer);appTimer=null;}if(gpioTimer){clearInterval(gpioTimer);gpioTimer=null;}}

// =====================================================================
//  App: Bluetooth  (dispositivos / radar / salvos / GATT)
// =====================================================================
let bleTab='dispositivos', bleDetail=null, bleGatt=null, gattVal={};

function stopAppTimer(){if(appTimer){clearInterval(appTimer);appTimer=null;}}
function loadModulos(){
  const key=viewKey;
  $('view').innerHTML='<div class="page"><h2>Módulos</h2><div class="card" data-tab="3">Carregando…</div></div>';
  (async()=>{
    const cfg=await apiGet('config');
    if(viewKey!==key||!cfg)return;
    $('view').innerHTML='<div class="page"><h2>Módulos</h2>'+
     modCard('speaker','Alto-falante externo','Emite beep de scan no alto-falante (terminais + e − da bobina)',
       [row('Pino POS.(GPIO)',text('modSpkPos',cfg.scanSpeakPosPin?String(cfg.scanSpeakPosPin):'12')),
        row('Pino NEG.(GPIO)',text('modSpkNeg',cfg.scanSpeakNegPin?String(cfg.scanSpeakNegPin):'13')),
        row('Tom (Hz)',text('modSpkHZ',cfg.scanTone_hz?String(cfg.scanTone_hz):'2200'))],
       '<button class="btn" id="modSpkTest">Testar beep</button>')+
modCard('ledext','LED externo','Acende um LED externo no radar (2 terminais, mínimo comum).<br>Se desligado e "Busca intermitente" ligado: verifica BLE periodicamente; se achar dispositivo, pisca LED interno na distância; se não achar, espera 30s.',
        [chk('modExtLedOn','LED externo habilitado',cfg.extLedOn),
         chk('modExtLedInv','Inverter sinal (negativo ativo)',cfg.extLedInvert),
         row('Pino POS.(GPIO)',text('modExtLedPos',cfg.extLedPosPin?String(cfg.extLedPosPin):'2')),
         row('Pino NEG.(GPIO)',text('modExtLedNeg',cfg.extLedNegPin?String(cfg.extLedNegPin):'0')),
         chk('modIntScan','Busca intermitente (LED externo desligado)',cfg.intermittentScan),
         row('Intervalo (ms)',text('modIntInterval',cfg.intermittentIntervalMs?String(cfg.intermittentIntervalMs):'30000'))],
        '<button class="btn" id="modLedTest">Piscar LED</button>')+
     modCard('motor','Motor DC 3V','Motor com detecção de rotação e calibração direita/esquerda',
       [row('Pino POS.(GPIO)',text('modMotPos',cfg.scanSpeakPosPin?String(cfg.scanSpeakPosPin):'12')),
        row('Pino NEG.(GPIO)',text('modMotNeg',cfg.scanSpeakNegPin?String(cfg.scanSpeakNegPin):'13'))],
       '<button class="btn" id="modMotWiz">Setup do Motor (wizard)</button>')+
     '<div class="card" data-tab="3"><button class="btn" id="modSave" style="width:100%">Salvar configuração</button></div>'+
     '</div>';
    $('modSpkTest').onclick=async()=>{
      const r=await apiPost('self-test',{});
      toast(r&&r.ok?'Beep disparado (3x)':'Self-test não disponível');
    };
    $('modLedTest').onclick=async()=>{
      const r=await apiPost('self-test',{});
      toast(r&&r.ok?'LED externo piscado (3x)':'Self-test não disponível');
    };
    $('modMotWiz').onclick=async()=>startMotorWizard();
    $('modSave').onclick=async()=>{
      const body={scanSpeakPosPin:+$('modSpkPos').value,scanSpeakNegPin:+$('modSpkNeg').value,
        scanTone_hz:+$('modSpkHZ').value,
        extLedOn:$('modExtLedOn').checked,extLedInvert:$('modExtLedInv').checked,
        extLedPosPin:+$('modExtLedPos').value,extLedNegPin:+$('modExtLedNeg').value,
        intermittentScan:$('modIntScan').checked,intermittentIntervalMs:+$('modIntInterval').value,
        scanSpeakPosPin:+$('modMotPos').value,scanSpeakNegPin:+$('modMotNeg').value};
      const r=await apiPost('config',body);
      toast(r&&r.ok?'Configuração salva':'Falha ao salvar');
    };
  })();
}
function startMotorWizard(){
  const key=viewKey;
  $('view').innerHTML='<div class="page"><h2>Setup do Motor</h2><div class="card" data-tab="3">Etapa 1 — informe os pinos</div></div>';
  (async()=>{
    const cfg=await apiGet('config');
    if(viewKey!==key||!cfg)return;
    $('view').innerHTML='<div class="page"><h2>Setup do Motor</h2>'+
      '<div class="card" data-tab="3"><h3>Etapa 1 de 3 — Pinos</h3>'+
       row('Pino POS.(GPIO)',text('motPos',cfg.scanSpeakPosPin?String(cfg.scanSpeakPosPin):'12'))+
       row('Pino NEG.(GPIO)',text('motNeg',cfg.scanSpeakNegPin?String(cfg.scanSpeakNegPin):'13'))+
      '<button class="btn" id="motNext1" style="width:100%">Próximo →</button></div>';
    $('motNext1').onclick=async()=>{motorStep2();};
  })();
}
function motorStep2(){
  const key=viewKey;
  $('view').innerHTML='<div class="page"><h2>Setup do Motor</h2>'+
    '<div class="card" data-tab="3"><h3>Etapa 2 de 3 — Gire o motor</h3>'+
     '<p class="hint">Gire o eixo do motor com a mão (como se gerasse energia). O ESP32 lê a voltagem gerada (back-EMF) nos 2 pinos.</p>'+
     '<div id="motProbeOut" class="hint">Aguardando…</div>'+
    '<button class="btn" id="motDoProbe" style="width:100%">Detectar rotação</button>'+
    '<button class="btn" id="motBack2" style="width:48%">← Voltar</button></div>';
  $('motBack2').onclick=async()=>startMotorWizard();
  $('motDoProbe').onclick=async()=>{
    const b=document.activeElement; if(b)b.disabled=true;
    const r=await apiGet('motor/probe');
    if(b)b.disabled=false;
    if(r&&r.ok){$('motProbeOut').innerHTML=(r.resolved?'<b class="ok">Motor detectado!</b>':'<b class="err">Nada detectado — confira os pinos e tente de novo.</b>')+
      ' <span class="hint">transições: '+r.transitions+' · pico: '+r.peak+'</span>';
      if(r.resolved){$('motProbeOut').innerHTML+='<br><button class="btn" id="motNext2" style="margin-top:8px">Próximo → calibrar direita</button>';
        $('motNext2').onclick=async()=>motorStep3();}
    }
  };
}
function motorStep3(){
  const key=viewKey;
  $('view').innerHTML='<div class="page"><h2>Setup do Motor</h2>'+
    '<div class="card" data-tab="3"><h3>Etapa 3 de 3 — Calibrar direita</h3>'+
     '<p class="hint">Gire o motor para a <b>DIREITA</b> agora e segure. Vou registrar qual pino fica positivo nessa rotação.</p>'+
     '<div id="motCalOut" class="hint">Girando…</div>'+
    '<button class="btn" id="motDoCal" style="width:100%">Começar calibração</button>'+
    '<button class="btn" id="motBack3" style="width:48%">← Voltar</button></div>';
  $('motBack3').onclick=async()=>motorStep2();
  $('motDoCal').onclick=async()=>{
    const b=document.activeElement; if(b)b.disabled=true;
    const r=await apiGet('motor/probe');
    if(b)b.disabled=false;
    if(r&&r.ok){$('motCalOut').innerHTML=(r.resolved?'Calibrado — motor vai para a <b>direita</b> quando o pino POS está alto.<br>':'Repita girando para a direita.')+
      '<span class="hint">'+(r.peak>0?'pico '+r.peak+' · '+r.transitions+' transições':'')+'</span>';
      if(r.resolved){$('motCalOut').innerHTML+='<button class="btn" id="motFinish" style="margin-top:8px">Concluir e salvar ✓</button>';
        $('motFinish').onclick=async()=>{
          const body={scanSpeakPosPin:+$('motPos').value,scanSpeakNegPin:+$('motNeg').value};
          const r=await apiPost('config',body);
          toast(r&&r.ok?'Motor configurado e salvo':'Falha ao salvar');
          apiGet('state').then(s=>{if(s&&s.inApp)stopApp();});
        };
      }
    }
  };
}

function modCard(id,title,desc,rowsHtml,testBtn){
  return '<div class="card" data-tab="3"><h3>'+title+'</h3><p class="hint">'+desc+'</p>'+
   '<div class="row"><label>Pinos</label><div class="cols">'+rowsHtml.join('')+'</div></div>'+
   testBtn+'</div>';
}

function rssiBars(r){
  const l=r>=-55?4:r>=-67?3:r>=-75?2:r>=-85?1:0;
  return '<span class="bars">'+[1,2,3,4].map(i=>'<i style="height:'+(4+i*3)+'px" class="'+(i<=l?'on':'')+'"></i>').join('')+'</span>';
}
function bleTabsHtml(){
  const t=[['dispositivos','Dispositivos'],['radar','Radar'],['meus','Meus']];
  return '<div class="tabs">'+t.map(x=>'<button data-action="ble-tab" data-tab="'+x[0]+'" class="'+(bleTab===x[0]?'active':'')+'">'+x[1]+'</button>').join('')+'</div>';
}
function renderBlePage(){
  $('view').innerHTML='<div class="page"><h2>Bluetooth</h2>'+bleTabsHtml()+'<div id="bleBody"><div class="card" data-tab="3">Carregando…</div></div></div>';
  refreshBle();
}
function loadBle(){
  bleTab='dispositivos';bleDetail=null;bleGatt=null;gattVal={};
  renderBlePage();
  apiPost('ble/scan',{active:true});
  appTimer=setInterval(refreshBle,2000);
}
async function refreshBle(){
  if(bleGatt){const st=await apiGet('ble/gatt/status');if(st&&st.on&&st.key){gattVal[st.key]={on:true,hex:st.hex,ascii:st.ascii};}renderGatt();return;}
  if(bleDetail){await renderBleDetail();return;}
  if(bleTab==='meus'){const s=await apiGet('ble/saved');paintSaved(s);return;}
  const s=await apiGet('ble/scan');
  if(!s)return;
  if(bleTab==='radar')paintRadar(s);else paintDevices(s);
}
function itemHtml(d){
  return '<div class="item" data-action="ble-open" data-addr="'+d.addr+'" data-name="'+esc(d.name)+'">'+
    '<div style="flex:1;min-width:0"><div class="nm">'+esc(d.name||'Sem nome')+(d.saved?' <span class="savedbadge">salvo</span>':'')+'</div>'+
    '<div class="sub">'+d.addr+'</div></div>'+rssiBars(d.rssi)+'<span class="rssi">'+d.rssi+'</span></div>';
}
function paintDevices(s){
  const el=$('bleBody');if(!el)return;
  let h='<div class="card" style="display:flex;align-items:center;justify-content:space-between">'+
    '<div><b>'+s.devices.length+'</b> dispositivo(s)<div class="mini">'+(s.scanning?'Escaneando…':'Scan parado')+'</div></div>'+
    '<button class="tagbtn '+(s.scanning?'':'solid')+'" data-action="ble-scan" data-on="'+(s.scanning?0:1)+'">'+(s.scanning?'Parar':'Escanear')+'</button></div>';
  if(!s.devices.length)h+='<div class="card" data-tab="3"><p class="hint">Nenhum dispositivo por perto. Toque em <b>Escanear</b>.</p></div>';
  h+='<div class="list">'+s.devices.map(itemHtml).join('')+'</div>';
  el.innerHTML=h;
}
function paintRadar(s){
  const el=$('bleBody');if(!el)return;
  const devs=s.devices.slice().sort((a,b)=>b.rssi-a.rssi);
  let h='<div class="card" data-tab="3" style="display:flex;align-items:center;justify-content:space-between">'+
    '<div><b>'+devs.length+'</b> dispositivo(s) por distância<div class="mini">'+(s.scanning?'Escaneando…':'Scan parado')+'</div></div>'+
    '<button class="tagbtn '+(s.scanning?'':'solid')+'" data-action="ble-scan" data-on="'+(s.scanning?0:1)+'">'+(s.scanning?'Parar':'Escanear')+'</button></div>';
  if(!devs.length){
    h+='<div class="card" data-tab="3"><p class="hint">Nenhum dispositivo por perto. Toque em <b>Escanear</b>.</p></div>';
  }else{
    h+='<div class="list">';
    devs.forEach(d=>{
      const rssi=d.rssi;
      let dist='—';
      if(rssi>=-35)dist='<b>colado (~0.5m)</b>';
      else if(rssi>=-50)dist='<b>perto (~1-2m)</b>';
      else if(rssi>=-70)dist='<b>médio (~3-5m)</b>';
      else if(rssi>=-85)dist='<b>longe (~6-10m)</b>';
      else dist='<b>muito longe (>10m)</b>';
      const saved=d.saved?' <span class="savedbadge">salvo</span>':'';
      h+='<div class="item" data-action="ble-open" data-addr="'+d.addr+'" data-name="'+esc(d.name)+'">'+
        '<div style="flex:1;min-width:0">'+
          '<div class="nm">'+esc(d.name||'Sem nome')+saved+'</div>'+
          '<div class="sub">'+d.addr+' • '+dist+'</div>'+
        '</div>'+rssiBars(rssi)+'<span class="rssi">'+rssi+' dBm</span></div>';
    });
    h+='</div>';
  }
  el.innerHTML=h;
}
function paintSaved(s){
  const el=$('bleBody');if(!el)return;
  const d=(s&&s.devices)||[];
  let h='<div class="card" data-tab="3"><b>'+d.length+'</b> dispositivo(s) salvo(s)<div class="mini">Toque para abrir, use Esquecer para remover.</div></div>';
  if(!d.length)h+='<div class="card" data-tab="3"><p class="hint">Nenhum dispositivo salvo. Abra um dispositivo em <b>Dispositivos</b> e toque em Salvar.</p></div>';
  h+='<div class="list">'+d.map(x=>'<div class="item" data-action="ble-open" data-addr="'+x.addr+'" data-name="'+esc(x.name)+'">'+
    '<div style="flex:1;min-width:0"><div class="nm">'+esc(x.name||'Sem nome')+'</div><div class="sub">'+x.addr+'</div></div>'+
    '<button class="tagbtn" data-action="ble-forget" data-addr="'+x.addr+'">Esquecer</button></div>').join('')+'</div>';
  el.innerHTML=h;
}
async function renderBleDetail(){
  const el=$('bleBody');if(!el)return;
  const s=await apiGet('ble/scan');
  const cur=(s&&s.devices.find(x=>x.addr===bleDetail.addr))||bleDetail;
  let h='<button class="tagbtn" data-action="ble-back" style="margin-bottom:10px">← Voltar</button>';
  h+='<div class="card" data-tab="3"><div class="nm" style="font-size:17px">'+esc(cur.name||'Sem nome')+'</div><div class="kv" style="margin-top:8px">'+
     kv('Endereço',cur.addr)+kv('Tipo',cur.type===1?'aleatório':'público')+
     kv('Sinal',(cur.rssi!==undefined?cur.rssi:'—')+' dBm')+kv('Salvo',cur.saved?'sim':'não')+'</div></div>';
  if(cur.svc&&cur.svc.length){h+='<div class="card" data-tab="3"><h3>Serviços anunciados</h3>'+cur.svc.map(u=>'<div class="mini" style="word-break:break-all">'+esc(u)+'</div>').join('')+'</div>';}
  h+='<div class="card" data-tab="3"><h3>Gerenciar</h3>'+
     (cur.saved?'<button class="btn danger" data-action="ble-forget" data-addr="'+cur.addr+'">Esquecer dispositivo</button>'
               :'<button class="btn" data-action="ble-save" data-addr="'+cur.addr+'" data-name="'+esc(cur.name)+'">Salvar dispositivo</button>')+
     '<button class="btn outlined" data-action="ble-gatt" data-addr="'+cur.addr+'">Comandos GATT</button></div>';
  el.innerHTML=h;
}
async function openGatt(addr){
  bleDetail=null;bleGatt={addr:addr,svcs:[]};
  const el=$('bleBody');if(el)el.innerHTML='<div class="card" data-tab="3">Conectando e descobrindo serviços…</div>';
  apiPost('ble/scan',{active:false});
  const r=await apiPost('ble/gatt',{addr:addr});
  if(!r||r.error){if(el)el.innerHTML='<div class="card warn">Falha ao conectar: '+esc(r?r.error:'sem resposta')+'</div><button class="tagbtn" data-action="ble-back" style="margin-top:8px">← Voltar</button>';return;}
  bleGatt.svcs=r.services||[];
  renderGatt();
}
function renderGatt(){
  const el=$('bleBody');if(!el||!bleGatt)return;
  const g=bleGatt;
  let h='<button class="tagbtn" data-action="ble-back" style="margin-bottom:10px">← Voltar</button>';
  h+='<div class="card"><h3>Serviços de '+esc(g.addr)+'</h3><p class="hint">Leia, escreva (hex) ou assine notificações. Mantenha a tela aberta para ouvir.</p></div>';
  if(!g.svcs.length)h+='<div class="card warn">Nenhum serviço encontrado.</div>';
  g.svcs.forEach(sv=>{
    h+='<div class="gattbox"><div class="cu">'+esc(sv.uuid)+'</div>';
    (sv.chars||[]).forEach(ch=>{
      const props=[];if(ch.read)props.push('read');if(ch.write)props.push('write');if(ch.noResp)props.push('writeNR');if(ch.notify)props.push('notify');if(ch.indicate)props.push('indicate');
      const key=sv.uuid+'|'+ch.uuid, v=gattVal[key];
      h+='<div style="margin-top:8px"><div class="cu" style="color:var(--text)">'+esc(ch.uuid)+'</div><div class="ch">'+
         props.map(p=>'<span class="pill'+(p==='notify'&&v&&v.on?' on':'')+'">'+p+'</span>').join('')+
         (ch.read?'<button class="tagbtn" data-action="gatt-read" data-svc="'+sv.uuid+'" data-chr="'+ch.uuid+'">Ler</button>':'')+
         (ch.write||ch.noResp?'<button class="tagbtn" data-action="gatt-write" data-svc="'+sv.uuid+'" data-chr="'+ch.uuid+'">Escrever</button>':'')+
         (ch.notify||ch.indicate?'<button class="tagbtn '+(v&&v.on?'solid':'')+'" data-action="gatt-notify" data-svc="'+sv.uuid+'" data-chr="'+ch.uuid+'" data-on="'+(v&&v.on?0:1)+'">'+(v&&v.on?'Parar':'Ouvir')+'</button>':'')+
         '</div>';
      if(v&&v.hex!==undefined)h+='<div class="log">'+esc(v.hex)+(v.ascii?'<br><span>'+esc(v.ascii)+'</span>':'')+'</div>';
      h+='</div>';
    });
    h+='</div>';
  });
  el.innerHTML=h;
}

// =====================================================================
//  App: WiFi Monitor  (apenas metadados)
// =====================================================================
let wmTab='live';
function stat(l,v){return '<div class="stat"><b>'+v+'</b><span>'+l+'</span></div>';}
function renderWmPage(){
  $('view').innerHTML='<div class="page"><h2>WiFi Monitor</h2><div class="tabs">'+
   [['live','Ao vivo'],['redes','Redes'],['macs','MACs']].map(x=>'<button data-action="wm-tab" data-tab="'+x[0]+'" class="'+(wmTab===x[0]?'active':'')+'">'+x[1]+'</button>').join('')+
   '</div><div id="wmBody"><div class="card">Carregando…</div></div></div>';
}
function loadWifiMon(){wmTab='live';renderWmPage();refreshWm();appTimer=setInterval(refreshWm,1000);}
async function refreshWm(){
  const d=await apiGet('wifi/monitor');
  if(!d)return;
  const el=$('wmBody');if(!el)return;
  if(wmTab==='live')paintWmLive(d,el);
  else if(wmTab==='redes')paintWmNets(d,el);
  else paintWmMacs(d,el);
}
function paintWmLive(d,el){
  el.innerHTML=
  '<div class="card" style="display:flex;align-items:center;justify-content:space-between">'+
   '<div><b>'+(d.on?'Monitorando':'Parado')+'</b><div class="mini">canal '+(d.ch||'—')+' • '+d.pps+' pacotes/s</div></div>'+
   '<button class="tagbtn '+(d.on?'':'solid')+'" data-action="wm-toggle" data-on="'+(d.on?0:1)+'">'+(d.on?'Parar':'Iniciar')+'</button></div>'+
  '<div class="stats">'+stat('Total',d.total)+stat('Pacotes/s',d.pps)+stat('Beacons',d.beacon)+stat('Probes',d.probe)+
   stat('Dados',d.data)+stat('Gerenciamento',d.mgmt)+stat('Controle',d.ctrl)+stat('Outros',d.misc)+'</div>'+
  '<div class="card" style="margin-top:12px"><h3>Sinal</h3><div class="kv">'+
   kv('RSSI atual',d.rssi+' dBm')+kv('RSSI mín',d.rssiMin+' dBm')+kv('RSSI máx',d.rssiMax+' dBm')+kv('Canal',d.ch||'—')+'</div></div>'+
  '<div class="card"><h3>Varredura de canais</h3>'+chk('wmHop','Pular canais 1–13 (pode derrubar a conexão WiFi)',d.hop)+
   '<p class="hint">Sem varredura o rádio fica no canal atual. Com varredura o monitor vê mais redes, mas a conexão STA pode cair.</p></div>'+
  '<div class="card warn"><p class="hint"><b>Privacidade:</b> este monitor exibe apenas metadados (endereços, canais, sinal e contadores). Nenhum conteúdo, payload ou senha é capturado.</p></div>';
}
function paintWmNets(d,el){
  const nets=(d.nets||[]).slice().sort((a,b)=>b.rssi-a.rssi);
  let h='<div class="card"><b>'+nets.length+'</b> rede(s) vista(s) • canal '+(d.ch||'—')+'</div>';
  if(!nets.length)h+='<div class="card"><p class="hint">Nenhuma rede ainda. Inicie o monitor em <b>Ao vivo</b>.</p></div>';
  h+='<div class="list">'+nets.map(n=>'<div class="item">'+
    '<div style="flex:1;min-width:0"><div class="nm">'+esc(n.ssid||'(oculto)')+'</div>'+
    '<div class="sub">canal '+n.ch+' • '+n.beacons+' beacons • '+(n.secure?'protegida':'aberta')+'</div></div>'+
    rssiBars(n.rssi)+'<span class="rssi">'+n.rssi+'</span></div>').join('')+'</div>';
  el.innerHTML=h;
}
function paintWmMacs(d,el){
  const macs=(d.macs||[]).slice().sort((a,b)=>b.count-a.count);
  const kindName=k=>k===0?'gerenc.':k===1?'controle':k===2?'dados':'outro';
  let h='<div class="card"><b>'+macs.length+'</b> endereço(s) visto(s)</div>';
  if(!macs.length)h+='<div class="card"><p class="hint">Nenhum pacote ainda.</p></div>';
  h+='<div class="list">'+macs.map(m=>'<div class="item">'+
    '<div style="flex:1;min-width:0"><div class="nm" style="font-family:ui-monospace,monospace">'+m.mac+'</div>'+
    '<div class="sub">canal '+m.ch+' • '+kindName(m.kind)+' • '+m.count+' pacote(s)</div></div>'+
    '<span class="rssi">'+m.rssi+'</span></div>').join('')+'</div>';
  el.innerHTML=h;
}

function collectCfg(){
  return {deviceName:$('inpName').value.trim(),theme:$('selTheme').value,accent:hexToInt($('inpAccent').value),
   ledBrightness:+$('rngBri').value,ledInvert:$('ledInvert').checked,ledScanExt:$('ledScanExt').checked,scanSpeakPosPin:+$('inpScanPos').value,scanSpeakNegPin:+$('inpScanNeg').value,apEnabled:$('chkAP').checked,apShareInternet:$('inpApNat').checked,
   wifiHunter:$('chkWifiHunter').checked,hunterInterval:+$('inpHunterInterval').value,
   apSsid:$('inpApSsid').value.trim(),apPassword:$('inpApPass').value};
}
async function doSaveConfig(){
  const c=collectCfg();
  const r=await apiPost('config',c);
  if(r&&r.ok)toast('Configurações salvas');
  else toast('Erro ao salvar');
  await loadSettings();
}
async function doSaveSta(){
  const body={mode:'sta',ssid:$('inpStaSsid').value.trim(),password:$('inpStaPass').value};
  const r=await apiPost('wifi',body);
  toast(r&&r.ok?'Conectando à rede '+body.ssid+'…':'Falha ao configurar WiFi');
  await refresh();
}

document.addEventListener('click',async e=>{
  const el=e.target.closest('[data-action]');
  if(!el)return;
  const a=el.dataset.action,idx=el.dataset.index;
  if(a.substring(0,4)==='nav-'){
    const act=a.substring(4);
    const body=act==='goto'?{action:'goto',index:+idx}:{action:act};
    await apiPost('nav',body);
    await refresh();
  }
  else if(a==='save-config')await doSaveConfig();
  else if(a==='hunter-scan'){
    const btn=el; btn.disabled=true; btn.textContent='Escanando…';
    const r=await apiPost('wifi/hunter',{action:'scan'});
    btn.disabled=false; btn.textContent='Escanear agora';
    if(r&&r.ok)toast('Scan iniciado');
    else toast('Falha ao iniciar scan');
    setTimeout(refreshSettings,1000);
  }
  else if(a==='save-config')await doSaveConfig();
  else if(a==='self-test'){
    const btn=el; btn.disabled=true; btn.textContent='Testando…';
    const r=await apiPost('self-test',{});
    btn.disabled=false; btn.textContent='Self-test LED (embutido + externo)';
    if(r&&r.ok)toast(r.result||'Teste concluído');
    else toast('Falha no self-test');
  }
  else if(a==='save-sta')await doSaveSta();
  else if(a==='save-notes'){
    const v=$('txtNotes').value;if(v.length>2000){toast('Máximo 2000 caracteres');return;}
    await apiPost('notes',{notes:v});toast('Notas salvas');
  }
  else if(a==='reboot'){if(confirm('Reiniciar o dispositivo?')){await apiPost('reboot',{});toast('Reiniciando…');setTimeout(()=>location.reload(),2600);}}
  else if(a==='reset'){if(confirm('Restaurar configurações de fábrica?')){await apiPost('reset',{});toast('Restaurando…');setTimeout(()=>location.reload(),3000);}}
  // --- Bluetooth ---
  else if(a==='ble-tab'){bleTab=el.dataset.tab;bleDetail=null;renderBlePage();}
  else if(a==='ble-scan'){await apiPost('ble/scan',{active:el.dataset.on==='1'});setTimeout(refreshBle,300);}
  else if(a==='ble-open'){bleDetail={addr:el.dataset.addr,name:el.dataset.name};renderBleDetail();}
  else if(a==='ble-back'){bleDetail=null;bleGatt=null;renderBlePage();}
  else if(a==='ble-save'){const r=await apiPost('ble/save',{addr:el.dataset.addr,name:el.dataset.name||''});toast(r&&r.ok?'Dispositivo salvo':'Erro ao salvar');refreshBle();}
  else if(a==='ble-forget'){const r=await apiPost('ble/forget',{addr:el.dataset.addr});toast(r&&r.ok?'Dispositivo esquecido':'Erro');refreshBle();}
  else if(a==='ble-gatt'){openGatt(el.dataset.addr);}
  else if(a==='gatt-read'){
    toast('Lendo…');
    const r=await apiPost('ble/gatt/read',{addr:bleGatt.addr,svc:el.dataset.svc,chr:el.dataset.chr});
    if(r&&!r.error){gattVal[el.dataset.svc+'|'+el.dataset.chr]={hex:r.hex,ascii:r.ascii};}
    else toast('Falha na leitura');
    renderGatt();
  }
  else if(a==='gatt-write'){
    const v=prompt('Valor em hexadecimal (ex.: 01A0):');
    if(v===null)return;
    const r=await apiPost('ble/gatt/write',{addr:bleGatt.addr,svc:el.dataset.svc,chr:el.dataset.chr,hex:v});
    toast(r&&r.ok?'Escrito':'Falha ao escrever');
  }
  else if(a==='gatt-notify'){
    const on=el.dataset.on==='1';
    const r=await apiPost('ble/gatt/notify',{addr:bleGatt.addr,svc:el.dataset.svc,chr:el.dataset.chr,enable:on});
    if(r&&!r.error){const k=el.dataset.svc+'|'+el.dataset.chr;gattVal[k]=gattVal[k]||{};gattVal[k].on=on;}else toast('Falha');
    renderGatt();
  }
  // --- WiFi Monitor ---
  else if(a==='wm-tab'){wmTab=el.dataset.tab;renderWmPage();refreshWm();}
  else if(a==='wm-toggle'){await apiPost('wifi/monitor',{on:el.dataset.on==='1'});toast(el.dataset.on==='1'?'Monitor ativado':'Monitor parado');setTimeout(refreshWm,500);}
});
document.addEventListener('change',e=>{
  if(e.target.id==='wmHop'){apiPost('wifi/monitor',{hop:e.target.checked});toast(e.target.checked?'Varredura de canais ativada':'Varredura desativada');}
});

async function boot(){
  cfg=await apiGet('config');
  applyTheme();
  await refresh();
  setInterval(refresh,1000);
}
boot();
</script>
</body>
</html>
)rawliteral";

// ------------------------------------------------------------------
// Página de LOGIN (PIN). Mostra o formulário quando não autenticado ou
// leva à criação de PIN quando ainda não configurado.
// ------------------------------------------------------------------
const char LOGIN_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#0f1420">
<title>ESPhone — Login</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{min-height:100vh;display:flex;align-items:center;justify-content:center;
       background:radial-gradient(1200px 600px at 20% -10%,#12202f,#0b0f18 60%);
       color:#e4ecf6;font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif;padding:16px}
  .box{background:#131a27;border:1px solid #253350;border-radius:16px;padding:28px;width:100%;max-width:320px;box-shadow:0 20px 60px rgba(0,0,0,.5)}
  h1{font-size:20px;margin-bottom:20px;text-align:center;letter-spacing:.5px}
  input{width:100%;padding:12px 14px;font-size:18px;letter-spacing:10px;text-align:center;
        background:#0b111c;color:#fff;border:1px solid #253350;border-radius:10px;outline:none;font-family:inherit}
  input:focus{border-color:#3fb8ec}
  button{width:100%;margin-top:16px;padding:12px;font-size:16px;font-weight:600;color:#0b111c;
         background:#3fb8ec;border:none;border-radius:10px;cursor:pointer;font-family:inherit}
  button:hover{background:#59c4f0}
  .msg{text-align:center;font-size:13px;margin-top:12px;min-height:18px;color:#9fb4cc}
  .err{color:#ff5b6e !important}
  .hint{margin-top:16px;font-size:12px;color:#8297b3;text-align:center;line-height:1.5}
</style>
</head>
<body>
<div class="box">
  <h1 id="tit">Verificando…</h1>
  <div class="msg" id="msg"></div>
  <div class="hint" id="hintA" style="display:none">Crie um PIN de 4 dígitos para proteger o acesso.</div>
  <div class="hint" id="hintB" style="display:none">Digite o PIN de 4 dígitos.</div>
</div>
<script>
var m=function(id){return document.getElementById(id);};
var mode='?';
var field=document.createElement('input');
field.type='password';field.id='pin';field.inputMode='numeric';field.maxLength=4;
field.autocomplete='current-password';field.style.cssText='width:100%;padding:12px 14px;font-size:18px;letter-spacing:10px;text-align:center;background:#0b111c;color:#fff;border:1px solid #253350;border-radius:10px;outline:none;font-family:inherit;margin-bottom:12px';
field.focus=function(){setTimeout(()=>HTMLInputElement.prototype.focus.call(this),10);};
var btn=document.createElement('button');
btn.style.cssText='width:100%;padding:12px;font-size:16px;font-weight:600;color:#0b111c;background:#3fb8ec;border:none;border-radius:10px;cursor:pointer;font-family:inherit';
window.sendPin=async function(){
  var pin=field.value.trim(),msg=m('msg');
  msg.className='msg';
  if(!/^\d{4}$/.test(pin)){msg.textContent='PIN deve ter 4 dígitos';msg.classList.add('err');return;}
  var url=mode==='setup'?'/api/pin/setup':'/api/pin/verify';
  var r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({pin:pin})});
  var j=await r.json();
  if(j&&j.ok){location.href='/';}else{msg.textContent=(j&&j.error==='invalid')?'PIN incorreto':(j&&j.error==='exists')?'PIN já configurado — digite o atual':(j&&j.error==='pin-not-set')?'Defina um PIN primeiro':'Erro';msg.classList.add('err');}
};
  field.addEventListener('keydown',function(e){if(e.key==='Enter')sendPin();});
  btn.onclick=sendPin;
  function paint(){
  var box=document.querySelector('.box');
  m('tit').textContent=mode==='setup'?'Crie seu PIN':'Digite o PIN';
  m('hintA').style.display=mode==='setup'?'block':'none';
  m('hintB').style.display=mode==='setup'?'none':'block';
  if(!document.getElementById('pin')){box.insertBefore(field,box.children[1]);box.insertBefore(btn,box.children[2]);}
  btn.textContent=mode==='setup'?'Criar PIN':'Entrar';
  field.placeholder=mode==='setup'?'4 dígitos':'••••';
  setTimeout(function(){field.focus();},50);
}
(async function(){
  try{
    var r=await fetch('/api/pin/status');
    var j=await r.json();
    mode=(j&&j.set)?'login':'setup';
  }catch(e){mode='login';}
  paint();
})();
</script>
</body>
</html>
)rawliteral";

#endif