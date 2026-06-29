#include "web_server.h"
#include "global_vars.h"

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <nvs_flash.h>
#include <esp_http_server.h>
#include <cstring>
#include <cstdio>

static const char *TAG = "web";
static httpd_handle_t server = NULL;
static int ws_client_fd = -1;

// --- Embedded web UI (minified from web.html) ---
static const char* HTML_INDEX = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<title>ESP32-P4 Digital Mixer (8 Channel)</title>
<style>
body{font-family:Arial,sans-serif;background:#111;color:#eee;margin:0;padding:0}
header{padding:10px 20px;background:#222;display:flex;justify-content:space-between;align-items:center}
.status{font-size:.9rem}
main{padding:10px;display:flex;flex-direction:row;gap:10px;overflow-x:auto}
.panel{background:#1b1b1b;padding:10px;border-radius:8px;min-width:220px}
.panel h2,.panel h3{margin:4px 0;font-size:1rem;text-align:center}
.slider-row{display:flex;align-items:center;justify-content:space-between;margin:3px 0;gap:5px}
.slider-row label{flex:0 0 70px;font-size:.8rem}
.slider-row input[type=range]{flex:1}
.slider-row span.value{width:40px;text-align:right;font-size:.8rem}
.vu{width:24px;height:120px;background:#333;border-radius:4px;margin:5px auto;position:relative;overflow:hidden}
.vu-fill{position:absolute;bottom:0;left:0;width:100%;background:linear-gradient(to top,#0f0,#ff0,#f00);height:0%}
.channel-strip{min-width:260px}
.section-title{font-size:.8rem;margin-top:6px;margin-bottom:2px;border-top:1px solid #333;padding-top:4px;text-align:center;opacity:.8}
input[type=range]{accent-color:#0af}
small{opacity:.7}
</style>
</head>
<body>
<header>
<div>ESP32-P4 Digital Mixer (8 Channel)</div>
<div class="status" id="wsStatus">WS: disconnected</div>
</header>
<main>
<div class="panel" id="masterPanel">
<h2>MASTER</h2>
<div class="vu" data-vu="masterL"><div class="vu-fill"></div></div>
<div style="text-align:center;font-size:.8rem">L</div>
<div class="vu" data-vu="masterR"><div class="vu-fill"></div></div>
<div style="text-align:center;font-size:.8rem">R</div>
<h3>Master L/R</h3>
<div class="slider-row"><label>Out L</label><input type="range" min="0" max="1" step="0.01" data-master="lrL"><span class="value">0.80</span></div>
<div class="slider-row"><label>Out R</label><input type="range" min="0" max="1" step="0.01" data-master="lrR"><span class="value">0.80</span></div>
<h3>Group</h3>
<div class="slider-row"><label>Group 1</label><input type="range" min="0" max="1" step="0.01" data-master="group1"><span class="value">0.80</span></div>
<div class="slider-row"><label>Group 2</label><input type="range" min="0" max="1" step="0.01" data-master="group2"><span class="value">0.80</span></div>
<h3>Aux</h3>
<div class="slider-row"><label>Aux 1</label><input type="range" min="0" max="1" step="0.01" data-master="aux1"><span class="value">0.80</span></div>
<div class="slider-row"><label>Aux 2</label><input type="range" min="0" max="1" step="0.01" data-master="aux2"><span class="value">0.80</span></div>
<h3>VU Bus Lain</h3>
<div class="vu" data-vu="group1"><div class="vu-fill"></div></div>
<div style="text-align:center;font-size:.8rem">Group 1</div>
<div class="vu" data-vu="group2"><div class="vu-fill"></div></div>
<div style="text-align:center;font-size:.8rem">Group 2</div>
<div class="vu" data-vu="aux1"><div class="vu-fill"></div></div>
<div style="text-align:center;font-size:.8rem">Aux 1</div>
<div class="vu" data-vu="aux2"><div class="vu-fill"></div></div>
<div style="text-align:center;font-size:.8rem">Aux 2</div>
</div>
<div id="channelsContainer" style="display:flex;gap:10px"></div>
</main>
<script>
const NUM_CHANNELS=8,NUM_EQ_BANDS=15;
const state={master:{lrL:.8,lrR:.8,group1:.8,group2:.8,aux1:.8,aux2:.8},channels:[]};
for(let ch=0;ch<NUM_CHANNELS;ch++)state.channels.push({eq:new Array(NUM_EQ_BANDS).fill(0),send:{lr:ch<2?1:.8,group1:0,group2:0,aux1:0,aux2:0},reverb:0,comp:{ratio:2,attack:10,release:200,threshold:-20,gain:0}});
const cc=document.getElementById('channelsContainer');
function createSlider(label,min,max,step,init,onChange){
 const row=document.createElement('div');row.className='slider-row';
 const l=document.createElement('label');l.textContent=label;row.appendChild(l);
 const inp=document.createElement('input');inp.type='range';inp.min=min;inp.max=max;inp.step=step;inp.value=init;row.appendChild(inp);
 const sp=document.createElement('span');sp.className='value';sp.textContent=(max-min>2)?init.toFixed(0):init.toFixed(2);row.appendChild(sp);
 inp.addEventListener('input',()=>{const v=parseFloat(inp.value);sp.textContent=(max-min>2)?v.toFixed(0):v.toFixed(2);onChange(v);scheduleSend();});
 return row;
}
function createChannel(ch){
 const p=document.createElement('div');p.className='panel channel-strip';
 const t=document.createElement('h2');t.textContent='CH '+(ch+1);p.appendChild(t);
 const vu=document.createElement('div');vu.className='vu';vu.dataset.vu='ch'+ch;
 const vf=document.createElement('div');vf.className='vu-fill';vu.appendChild(vf);p.appendChild(vu);
 const eqT=document.createElement('div');eqT.className='section-title';eqT.textContent='EQ 15-Band';p.appendChild(eqT);
 const freqs=[31,63,125,250,500,1000,2000,4000,6000,8000,10000,12000,14000,16000,18000];
 freqs.forEach((f,idx)=>{p.appendChild(createSlider(f+' Hz',-12,12,.5,0,v=>{state.channels[ch].eq[idx]=v;}));});
 const rT=document.createElement('div');rT.className='section-title';rT.textContent='Routing (Send Level)';p.appendChild(rT);
 p.appendChild(createSlider('To L/R',0,1,.01,ch<2?1:.8,v=>state.channels[ch].send.lr=v));
 p.appendChild(createSlider('To Group1',0,1,.01,0,v=>state.channels[ch].send.group1=v));
 p.appendChild(createSlider('To Group2',0,1,.01,0,v=>state.channels[ch].send.group2=v));
 p.appendChild(createSlider('To Aux1',0,1,.01,0,v=>state.channels[ch].send.aux1=v));
 p.appendChild(createSlider('To Aux2',0,1,.01,0,v=>state.channels[ch].send.aux2=v));
 const revT=document.createElement('div');revT.className='section-title';revT.textContent='Reverb';p.appendChild(revT);
 p.appendChild(createSlider('Amount',0,1,.01,0,v=>state.channels[ch].reverb=v));
 const cmpT=document.createElement('div');cmpT.className='section-title';cmpT.textContent='Compressor';p.appendChild(cmpT);
 p.appendChild(createSlider('Ratio',1,10,.1,2,v=>state.channels[ch].comp.ratio=v));
 p.appendChild(createSlider('Attack (ms)',0,100,1,10,v=>state.channels[ch].comp.attack=v));
 p.appendChild(createSlider('Release (ms)',10,1000,10,200,v=>state.channels[ch].comp.release=v));
 p.appendChild(createSlider('Thresh (dB)',-60,0,1,-20,v=>state.channels[ch].comp.threshold=v));
 p.appendChild(createSlider('Gain (dB)',-12,12,.5,0,v=>state.channels[ch].comp.gain=v));
 return p;
}
for(let ch=0;ch<NUM_CHANNELS;ch++)cc.appendChild(createChannel(ch));
document.querySelectorAll('#masterPanel input[type=range]').forEach(inp=>{
 const key=inp.dataset.master;inp.value=state.master[key];
 const sp=inp.parentElement.querySelector('.value');sp.textContent=state.master[key].toFixed(2);
 inp.addEventListener('input',()=>{const v=parseFloat(inp.value);state.master[key]=v;sp.textContent=v.toFixed(2);scheduleSend();});
});
let ws=null,sendTimer=null;
function connectWS(){
 const el=document.getElementById('wsStatus');
 ws=new WebSocket('ws://'+location.host+'/ws');
 ws.onopen=()=>{el.textContent='WS: connected';el.style.color='#0f0';sendParams();};
 ws.onclose=()=>{el.textContent='WS: disconnected';el.style.color='#f00';setTimeout(connectWS,2000);};
 ws.onerror=()=>{el.textContent='WS: error';el.style.color='#ff0';};
 ws.onmessage=(ev)=>{try{const m=JSON.parse(ev.data);if(m.type==='vu')updateVu(m);}catch(e){}};
}
function scheduleSend(){if(sendTimer)return;sendTimer=setTimeout(()=>{sendTimer=null;sendParams();},50);}
function sendParams(){if(!ws||ws.readyState!==WebSocket.OPEN)return;ws.send(JSON.stringify({type:'set',master:state.master,channels:state.channels}));}
function updateVu(msg){
 msg.channels.forEach((val,idx)=>{const vu=document.querySelector('.vu[data-vu="ch'+idx+'"] .vu-fill');if(vu)vu.style.height=(val*100).toFixed(1)+'%';});
 const bus=msg.bus||{};['masterL','masterR','group1','group2','aux1','aux2'].forEach(key=>{
  const vu=document.querySelector('.vu[data-vu="'+key+'"] .vu-fill');
  if(vu&&typeof bus[key]==='number')vu.style.height=(bus[key]*100).toFixed(1)+'%';
 });
}
connectWS();
</script>
</body></html>
)rawliteral";

// --- WebSocket frame helpers ---
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
  if (!wsStateChanged) {
    taskEXIT_CRITICAL(&wsMux);
    return;
  }
  wsStateChanged = false;
  // Build VU JSON
  DynamicJsonDocument doc(2048);
  doc["type"] = "vu";
  JsonArray chArr = doc.createNestedArray("channels");
  for (int ch = 0; ch < NUM_MIX_CH; ch++) {
    chArr.add(channels[ch].vu);
  }
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

// --- WebSocket handler ---
static esp_err_t ws_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    // WebSocket upgrade
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
    ESP_LOGE(TAG, "WS recv err %d", ret);
    if (ret == ESP_ERR_HTTPD_INVALID_REQ) {
      ws_client_fd = -1;
    }
    return ret;
  }

  if (frame.type == HTTPD_WS_TYPE_TEXT && frame.len > 0) {
    buf[frame.len] = 0;
    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, (const char*)buf);
    if (err) {
      ESP_LOGW(TAG, "WS JSON parse error");
      return ESP_OK;
    }

    const char *type = doc["type"];
    if (type && strcmp(type, "set") == 0) {
      // Master
      JsonObject mst = doc["master"];
      if (!mst.isNull()) {
        master.lrL = mst["lrL"] | master.lrL;
        master.lrR = mst["lrR"] | master.lrR;
        master.group1 = mst["group1"] | master.group1;
        master.group2 = mst["group2"] | master.group2;
        master.aux1 = mst["aux1"] | master.aux1;
        master.aux2 = mst["aux2"] | master.aux2;
      }

      // Channels
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

// --- REST API handlers (legacy compat) ---
static esp_err_t api_status_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  DynamicJsonDocument doc(8192);
  doc["sr"] = SampleRateFreq;
  doc["ch"] = NUM_MIX_CH;
  doc["ws"] = (ws_client_fd >= 0);
  doc["il"] = InputLevel;
  doc["ol"] = OutputLevel;
  doc["comp"] = Compressor;
  doc["mute"] = Mute;
  String output;
  serializeJson(doc, output);
  httpd_resp_sendstr(req, output.c_str());
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

// --- URI registration ---
static const httpd_uri_t uri_index = {
  .uri = "/", .method = HTTP_GET, .handler = index_handler
};
static const httpd_uri_t uri_ws = {
  .uri = "/ws", .method = HTTP_GET, .handler = ws_handler, .is_websocket = true
};
static const httpd_uri_t uri_api_status = {
  .uri = "/api/status", .method = HTTP_GET, .handler = api_status_handler
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
  config.max_uri_handlers = 10;
  config.stack_size = 8192;
  config.lru_purge_enable = true;
  if (httpd_start(&server, &config) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start HTTP server");
    return ESP_FAIL;
  }
  httpd_register_uri_handler(server, &uri_index);
  httpd_register_uri_handler(server, &uri_ws);
  httpd_register_uri_handler(server, &uri_api_status);
  httpd_register_uri_handler(server, &uri_api_save);
  httpd_register_uri_handler(server, &uri_api_reboot);
  ESP_LOGI(TAG, "Web server started on port %d", config.server_port);
  return ESP_OK;
}

esp_err_t stopWebServer() {
  if (server) {
    httpd_stop(server);
    server = NULL;
  }
  ws_client_fd = -1;
  return ESP_OK;
}

// --- WiFi Initialization ---
esp_err_t initWiFi() {
  ESP_LOGI(TAG, "Initializing WiFi (ESP32-C6 via SDIO)...");
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_ERROR_CHECK(esp_netif_init());
  ret = esp_event_loop_create_default();
  if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
    ESP_ERROR_CHECK(ret);
  }
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ret = esp_wifi_init(&cfg);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(ret));
    return ret;
  }
  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
    WIFI_EVENT, ESP_EVENT_ANY_ID,
    [](void*, esp_event_base_t, int32_t event_id, void*) {
      if (event_id == WIFI_EVENT_STA_START) esp_wifi_connect();
      else if (event_id == WIFI_EVENT_STA_DISCONNECTED) { esp_wifi_connect(); ESP_LOGI(TAG, "WiFi reconnecting..."); }
    }, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
    IP_EVENT, IP_EVENT_STA_GOT_IP,
    [](void*, esp_event_base_t, int32_t, void* data) {
      ip_event_got_ip_t *event = (ip_event_got_ip_t*)data;
      ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }, NULL, &instance_got_ip));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
  String ssid = readFile("/wifi.txt");
  String pass = "";
  if (ssid.isEmpty()) {
    ssid = "dsp-mixer";
    pass = "mixer1234";
    ESP_LOGW(TAG, "No /wifi.txt found, using AP mode: '%s'", ssid.c_str());
  }
  if (pass.isEmpty()) {
    int newline = ssid.indexOf('\n');
    if (newline > 0) {
      pass = ssid.substring(newline + 1); pass.trim();
      ssid = ssid.substring(0, newline); ssid.trim();
    }
  }
  wifi_config_t wifi_config = {};
  if (ssid == "dsp-mixer" && pass == "mixer1234") {
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    wifi_config_t ap_config = {};
    strncpy((char*)ap_config.ap.ssid, "AudioMixer-P4-8ch", sizeof(ap_config.ap.ssid) - 1);
    strncpy((char*)ap_config.ap.password, "mixer1234", sizeof(ap_config.ap.password) - 1);
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.max_connection = 4;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_LOGI(TAG, "AP mode: SSID 'AudioMixer-P4-8ch', password 'mixer1234'");
  } else {
    strncpy((char*)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid) - 1);
    if (!pass.isEmpty()) strncpy((char*)wifi_config.sta.password, pass.c_str(), sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_LOGI(TAG, "STA mode: SSID '%s'", ssid.c_str());
  }
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_LOGI(TAG, "WiFi init complete");
  return ESP_OK;
}
