#include <Arduino.h>
#include "driver/gpio.h"
#include "esp_task_wdt.h"
#include "esp_log.h"
#include <Preferences.h>
#include <sdkconfig.h>
#include <YummyDSP.h>
#include <AudioDriver.h>

#include "DualCompressor.h"
#include "settings.h"
#include "global_vars.h"
#include "serial_cmd.h"
#include "web_server.h"

Preferences preferences;

void webTask(void *pvParameters) {
  esp_task_wdt_add(NULL);
  const TickType_t xDelay = 20 / portTICK_PERIOD_MS;
  while (true) {
    webSocketBroadcastVu();
    esp_task_wdt_reset();
    vTaskDelay(xDelay);
  }
}

void setup() {
#if CONFIG_IDF_TARGET_ESP32P4
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = (uint32_t)WDT_TIMEOUT * 1000,
    .idle_core_mask = 0,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
#else
  esp_task_wdt_init(WDT_TIMEOUT, true);
#endif
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("===========================================");
  Serial.println("  8-CH DIGITAL AUDIO MIXER - ESP32-P4");
  Serial.println("  WebSocket Controlled v" + VERSION);
  Serial.println("===========================================");
  Serial.println();

  init_config();
  init_filters();
  readConfig();
  commitConfig();

  Serial.println();
  Serial.println("I2S Audio Config:");
  Serial.print("  Sample Rate: "); Serial.print(SampleRateFreq); Serial.println(" Hz");
  Serial.print("  Physical Inputs: "); Serial.println(NUM_PHYS_IN);
  Serial.print("  Mix Channels: "); Serial.println(NUM_MIX_CH);

  Serial.println("\n--- I2S (PCM1808 ADC + PCM5102 DAC) ---");
  Serial.printf("  BCK=%d LRCK=%d DOUT=%d DIN=%d\n",
    I2S_BCK_PIN, I2S_LRCLK_PIN, I2S_DOUT_PIN, I2S_DIN_PIN);

  i2sCodec.setI2sPort(I2S_PORT);
  i2sCodec.setFormat(SampleRateFreq, channelCount);
  i2sCodec.setPins(I2S_BCK_PIN, I2S_LRCLK_PIN, I2S_DOUT_PIN, I2S_DIN_PIN, CODEC_ENABLE_PIN, I2S0_MCLK_PIN);
  bool ok = i2sCodec.start();
  Serial.printf("\nI2S %s\n", ok ? "started successfully!" : "FAILED!");

  if (ok) {
    xTaskCreatePinnedToCore(audioTask, "audioTask", 16384, NULL, 10, NULL, 1);
    delay(500);
  } else {
    Serial.println("WARNING: I2S failed. Audio processing disabled.");
  }

  Serial.println();
  Serial.println("System ready! Type 'menu' for commands.");
  Serial.println();

  Serial.printf("Free heap before WiFi: %u bytes\n", esp_get_free_heap_size());
  esp_log_level_set("rpc_core", ESP_LOG_NONE);
  Serial.println("Starting WiFi...");
  initWiFi();
  startWebServer();
  Serial.println("Web server started! Connect to WiFi AP 'AudioMixer-8ch' or check router for IP.");

  delay(500);
  xTaskCreatePinnedToCore(serialCommTask, "serialCommTask", 4096, NULL, 6, NULL, 0);
  delay(500);
  xTaskCreatePinnedToCore(webTask, "webTask", 4096, NULL, 5, NULL, 0);
}

void loop() {
  delay(1000);
}
