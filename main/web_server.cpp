#include "web_server.h"
#include "mixer_state.h"

#include <WiFi.h>
#include <ETH.h>
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


static uint32_t lastVusend = 0;

static esp_err_t ws_send_text(const char *text) {
  if (ws_client_fd < 0) return ESP_FAIL;
  httpd_ws_frame_t frame = {};
  frame.type = HTTPD_WS_TYPE_TEXT;
  frame.payload = (uint8_t*)text;
  frame.len = strlen(text);
  esp_err_t ret = httpd_ws_send_frame_async(server, ws_client_fd, &frame);
  if (ret != ESP_OK) {
    ws_client_fd = -1;
  }
  return ret;
}

void webSocketBroadcastVu() {
  if (ws_client_fd < 0) return;
  uint32_t now = millis();
  if (now - lastVusend < 40) return;
  lastVusend = now;
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
  if (frame.type == HTTPD_WS_TYPE_CLOSE) {
    ws_client_fd = -1;
    return ESP_OK;
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

static const char *content_type_for(const char *path) {
  if (strstr(path, ".html")) return "text/html";
  if (strstr(path, ".js")) return "text/javascript";
  if (strstr(path, ".css")) return "text/css";
  if (strstr(path, ".woff2")) return "font/woff2";
  if (strstr(path, ".svg")) return "image/svg+xml";
  if (strstr(path, ".json")) return "application/json";
  if (strstr(path, ".txt")) return "text/plain";
  return "application/octet-stream";
}

/** Serves the static Next.js export from SPIFFS (web-ui/spiffs). */
static esp_err_t static_handler(httpd_req_t *req) {
  char path[128];
  snprintf(path, sizeof(path), "%s", req->uri);
  char *query = strchr(path, '?');
  if (query) *query = '\0';
  if (strcmp(path, "/") == 0) snprintf(path, sizeof(path), "/index.html");

  // Prevent path traversal and keep the server inside SPIFFS.
  if (strstr(path, "..")) {
    httpd_resp_send_404(req);
    return ESP_OK;
  }

  if (!SPIFFS.exists(path)) {
    snprintf(path, sizeof(path), "/404.html");
    if (!SPIFFS.exists(path)) {
      httpd_resp_send_404(req);
      return ESP_OK;
    }
  }

  File file = SPIFFS.open(path, "r");
  if (!file) {
    httpd_resp_send_404(req);
    return ESP_OK;
  }

  httpd_resp_set_type(req, content_type_for(path));
  if (strstr(path, "/_next/static/")) {
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=31536000, immutable");
  }

  uint8_t buffer[1024];
  size_t count;
  while ((count = file.read(buffer, sizeof(buffer))) > 0) {
    if (httpd_resp_send_chunk(req, (const char *)buffer, count) != ESP_OK) break;
  }
  httpd_resp_send_chunk(req, NULL, 0);
  file.close();
  return ESP_OK;
}

static const httpd_uri_t uri_index = {
  .uri = "/", .method = HTTP_GET, .handler = static_handler
};
static const httpd_uri_t uri_static = {
  .uri = "/*", .method = HTTP_GET, .handler = static_handler
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

esp_err_t initEthernet() {
  ESP_LOGI(TAG, "Initializing Ethernet (Arduino)...");
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    (void)info;
    switch (event) {
      case ARDUINO_EVENT_ETH_START:
        ESP_LOGI(TAG, "ETH start");
        ETH.setHostname("AudioMixer-8ch-ETH");
        break;
      case ARDUINO_EVENT_ETH_CONNECTED:
        ESP_LOGI(TAG, "ETH link up");
        break;
      case ARDUINO_EVENT_ETH_GOT_IP:
        ESP_LOGI(TAG, "Ethernet connected, IP: %s", ETH.localIP().toString().c_str());
        break;
      case ARDUINO_EVENT_ETH_DISCONNECTED:
        ESP_LOGI(TAG, "ETH link down");
        break;
      case ARDUINO_EVENT_ETH_STOP:
        ESP_LOGI(TAG, "ETH stopped");
        break;
      default:
        break;
    }
  });
  if (!ETH.begin()) {
    ESP_LOGE(TAG, "Failed to start Ethernet");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "Ethernet init complete");
  return ESP_OK;
}

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
  httpd_register_uri_handler(server, &uri_static);
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
