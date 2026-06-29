/*
  Project: 8-Channel Digital Audio Mixer
  Type: WebSocket Controlled Digital Mixer
  Microprocessor: ESP32-P4 (RISC-V)
  Features: 8ch mixing, 15-band EQ, compressor, reverb, 5 bus routing
  Board: ESP32-P4-DevKit or custom
  Framework: ESP-IDF with Arduino component
*/

#include <Arduino.h>
#include "driver/gpio.h"
#include "esp_task_wdt.h"
#include <Preferences.h>
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
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = (uint32_t)WDT_TIMEOUT * 1000,
    .idle_core_mask = 0,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);

  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("===========================================");
  Serial.println("  8-CH DIGITAL AUDIO MIXER - ESP32-P4");
  Serial.println("  WebSocket Controlled v" + VERSION);
  Serial.println("===========================================");
  Serial.println();

  init_config();
  readConfig();
  commitConfig();

  init_filters();

  Serial.println();
  Serial.println("I2S Audio Config:");
  Serial.print("  Sample Rate: "); Serial.print(SampleRateFreq); Serial.println(" Hz");
  Serial.print("  Physical Inputs: "); Serial.println(NUM_PHYS_IN);
  Serial.print("  Physical Outputs: "); Serial.println(NUM_PHYS_OUT);
  Serial.print("  Mix Channels: "); Serial.println(NUM_MIX_CH);

  Serial.println("\n--- I2S0 (ADC #1 + DAC #1) ---");
  Serial.printf("  BCK=%d LRCK=%d DOUT=%d DIN=%d\n", I2S_BCK_PIN, I2S_LRCLK_PIN, I2S_DOUT_PIN, I2S_DIN_PIN);

  int err = 0;
  i2sCodec0.setI2sPort(I2S_NUM_0);
  err += i2sCodec0.setFormat(SampleRateFreq, channelCount);
  err += i2sCodec0.setPins(I2S_BCK_PIN, I2S_LRCLK_PIN, I2S_DOUT_PIN, I2S_DIN_PIN, CODEC_ENABLE_PIN, I2S0_MCLK_PIN);
  err += i2sCodec0.start();

  Serial.println("\n--- I2S1 (ADC #2 + DAC #2) ---");
  Serial.printf("  BCK=%d LRCK=%d DOUT=%d DIN=%d\n", I2S1_BCK_PIN, I2S1_LRCLK_PIN, I2S1_DOUT_PIN, I2S1_DIN_PIN);

  i2sCodec1.setI2sPort(I2S_NUM_1);
  err += i2sCodec1.setFormat(SampleRateFreq, channelCount);
  err += i2sCodec1.setPins(I2S1_BCK_PIN, I2S1_LRCLK_PIN, I2S1_DOUT_PIN, I2S1_DIN_PIN, I2S1_ENABLE_PIN, I2S1_MCLK_PIN);
  err += i2sCodec1.start();

  if (err == 0) {
    Serial.println("\nBoth I2S ports started successfully!");
  } else {
    Serial.print("\nI2S error code: "); Serial.println(err);
  }

  Serial.println();
  Serial.println("System ready! Type 'menu' for commands.");
  Serial.println();

  xTaskCreatePinnedToCore(audioTask, "audioTask", 8192, NULL, 10, NULL, 1);
  delay(500);

  xTaskCreatePinnedToCore(serialCommTask, "serialCommTask", 4096, NULL, 6, NULL, 0);
  delay(500);

  xTaskCreatePinnedToCore(webTask, "webTask", 4096, NULL, 5, NULL, 0);
  delay(500);

  Serial.println("Initializing WiFi via ESP32-C6 co-processor...");
  if (initWiFi() == ESP_OK) {
    if (startWebServer() == ESP_OK) {
      Serial.println("Web server started! Connect to WiFi and open browser.");
    }
  } else {
    Serial.println("WiFi init failed. Continuing without network.");
  }
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
