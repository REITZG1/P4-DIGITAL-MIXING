#pragma once
#include <Arduino.h>

#ifndef I2S_MODE_MASTER
#define I2S_MODE_MASTER 1
#define I2S_MODE_SLAVE  2
#define I2S_MODE_TX     4
#define I2S_MODE_RX     8
#endif

#ifndef I2S_BITS_PER_SAMPLE_32BIT
#define I2S_BITS_PER_SAMPLE_32BIT ((i2s_bits_per_sample_t)32)
#endif
#ifndef I2S_COMM_FORMAT_I2S
#define I2S_COMM_FORMAT_I2S ((i2s_comm_format_t)1)
#endif
#ifndef I2S_COMM_FORMAT_I2S_MSB
#define I2S_COMM_FORMAT_I2S_MSB ((i2s_comm_format_t)2)
#endif

const String VERSION = "2.0.0 2026/06/29 P4-mixer8ch-ws";
#define CONFIG_TAG 125

#define SAVE_AS_JSON 1
#define ENG_LANG 1

const String APPLICATION_TITLE = "8-CH DIGITAL MIXER";

// Channel config
const int NUM_MIX_CH = 8;
const int NUM_EQ_BANDS = 15;
const int NUM_PHYS_IN = 4;
const int NUM_PHYS_OUT = 4;

// Bus IDs
const int BUS_LR = 0;
const int BUS_GROUP1 = 1;
const int BUS_GROUP2 = 2;
const int BUS_AUX1 = 3;
const int BUS_AUX2 = 4;
const int NUM_BUSES = 5;

// EQ frequencies (Hz)
const float EQ_FREQS[NUM_EQ_BANDS] = {
  31, 63, 125, 250, 500,
  1000, 2000, 4000, 6000, 8000,
  10000, 12000, 14000, 16000, 18000
};

// Compressor params per channel
struct CompressorParams {
  float ratio;
  float attack;
  float release;
  float threshold;
  float gain;
};

// Send levels per channel
struct ChannelSends {
  float lr;
  float group1;
  float group2;
  float aux1;
  float aux2;
};

// Per-channel state
struct MixChannel {
  float eq[NUM_EQ_BANDS];
  ChannelSends send;
  float reverb;
  CompressorParams comp;
  float vu;
  bool mute;
};

// Master bus levels
struct MasterState {
  float lrL;
  float lrR;
  float group1;
  float group2;
  float aux1;
  float aux2;
  float vuMasterL;
  float vuMasterR;
  float vuGroup1;
  float vuGroup2;
  float vuAux1;
  float vuAux2;
};

// Legacy compat from v1 (for multiband DSP)
const int MAX_NUM_BANDS = 10;
const int ALL_NUM_BANDS = MAX_NUM_BANDS + 2;
const int N_BAND = 10, N_FREQ = 11;

const int FILTER_FREQ[N_BAND][N_FREQ] = {
  {20, 18000, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {20, 6000, 18000, 0, 0, 0, 0, 0, 0, 0, 0},
  {20, 200, 3000, 18000, 0, 0, 0, 0, 0, 0, 0},
  {20, 200, 500, 17000, 18000, 0, 0, 0, 0, 0, 0},
  {20, 200, 500, 3000, 14000, 18000, 0, 0, 0, 0, 0},
  {20, 200, 500, 2000, 6000, 15000, 18000, 0, 0, 0, 0},
  {20, 200, 500, 2000, 6000, 14000, 16000, 18000, 0, 0, 0},
  {20, 200, 500, 2000, 6000, 14000, 16000, 17000, 18000, 0, 0},
  {20, 200, 500, 2000, 6000, 14000, 15000, 16000, 17000, 18000, 0},
  {20, 200, 500, 2000, 5000, 13000, 14000, 15000, 16000, 17000, 18000}
};

// Legacy DSP params (kept for multiband main bus)
const float MAX_INPUT_LEVEL = 90.0f;
const float MAX_OUTPUT_LEVEL = 10.0f;
const float MAX_BALANCE = 100.0f;
const float MAX_CLIPPER = 40.0f;
const float MAX_PRE_EMPHASIS = 30.0f;
const float MAX_POST_EMPHASIS = 30.0f;
const float MAX_STEP_BY = -3.0f;
const float MAX_ECHO = 1.0f;
const float MAX_PROTECTION = 30.0f;
const float MAX_GAIN = 80.0f;
const float MAX_ATTACK_TIME = 1000.0f;
const float MAX_RELEASE_TIME = 50000.0f;
const float MAX_EQ_BAND = 12.0f;
const float MAX_FILTERS_Q_FACTOR = 10.0f;

const float MIN_INPUT_LEVEL = -50.0f;
const float MIN_OUTPUT_LEVEL = -70.0f;
const float MIN_BALANCE = 0.0f;
const float MIN_CLIPPER = -40.0f;
const int MIN_NUM_BANDS = 1;
const float MIN_PRE_EMPHASIS = -30.0f;
const float MIN_POST_EMPHASIS = -30.0f;
const float MIN_STEP_BY = -90.0f;
const float MIN_ECHO = 0.0f;
const float MIN_PROTECTION = 0.0f;
const float MIN_GAIN = 0.0f;
const float MIN_ATTACK_TIME = 0.0f;
const float MIN_RELEASE_TIME = 5.0f;
const float MIN_EQ_BAND = -12.0f;
const float MIN_FILTERS_Q_FACTOR = 0.1f;

const float DEFAULT_INPUT_LEVEL = 16.0f;
const float DEFAULT_OUTPUT_LEVEL = -20.0f;
const float DEFAULT_BALANCE = 50.0f;
const float DEFAULT_CLIPPER = 0.0f;
const bool DEFAULT_COMPRESSOR = true;
const bool DEFAULT_BAND_SYNC = true;
const bool DEFAULT_MUTE = false;
const bool DEFAULT_RESERVED1 = false;
const bool DEFAULT_RESERVED2 = true;
const int DEFAULT_NUM_BANDS = 8;
const float DEFAULT_PRE_EMPHASIS = 0.0f;
const float DEFAULT_POST_EMPHASIS = -6.0f;
const float DEFAULT_STEP_BY = -14.0f;
const float DEFAULT_ECHO = 0.0f;
const float DEFAULT_PROTECTION = 7.0f;
const float DEFAULT_GAIN = 50.0f;
const float DEFAULT_ATTACK_TIME = 30.0f;
const float DEFAULT_RELEASE_TIME = 30.0f;
const float DEFAULT_EQ_BAND = 0.0f;
const float DEFAULT_FILTERS_Q_FACTOR = 1.0f;
const bool DEFAULT_FORCED_MUTE = true;
const int DEFAULT_FORCED_MUTE_TIME = 100;
const int DEFAULT_RESTART_TIME = 100;
const float DEFAULT_VOL_CORRECTION = -10.0f;
const float DEFAULT_VOL_COMPENSATION = 20.0f;

#define NumFmFilters 0
#define DEFAULT_CONFIG_FILENAME "/config.json"

const int SampleRateFreq = 48000;
const int channelCount = 2;

// I2S Pinout (ESP32-P4, shared BCK/LRCK per I2S port)
#define I2S_BCK_PIN       5
#define I2S_LRCLK_PIN     6
#define I2S_DOUT_PIN      7
#define I2S_DIN_PIN       4
#define I2S0_MCLK_PIN     21
#define CODEC_ENABLE_PIN  8

#define I2S1_BCK_PIN      15
#define I2S1_LRCLK_PIN    16
#define I2S1_DOUT_PIN     17
#define I2S1_DIN_PIN      10
#define I2S1_ENABLE_PIN   9
#define I2S1_MCLK_PIN     -1

#define WDT_TIMEOUT 10

static uint8_t CORE_ZERO = 0;
static uint8_t CORE_ONE = 1;
