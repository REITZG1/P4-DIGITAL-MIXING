#include "global_vars.h"
#include <cmath>

bool espRestart = false;
bool isGoodFileSystem = true;
uint32_t CounterLoopRestart_100ms = 0, RestartTimeX100ms = DEFAULT_RESTART_TIME;
uint16_t MuteTimeX100ms = DEFAULT_FORCED_MUTE_TIME;
int TimeZone = -3;

MixChannel channels[NUM_MIX_CH];
MasterState master;
float inTrim[NUM_PHYS_IN] = {0, 0};
bool inMute[NUM_PHYS_IN] = {false, false};
float inTrimLinear[NUM_PHYS_IN] = {1.0f, 1.0f};

float VuArrValue[MAX_NUM_BANDS];
float InputLevel = DEFAULT_INPUT_LEVEL;
float OutputLevel = DEFAULT_OUTPUT_LEVEL;
float Balance = DEFAULT_BALANCE;
float Clipper = DEFAULT_CLIPPER;
bool Compressor = DEFAULT_COMPRESSOR;
bool BandSync = DEFAULT_BAND_SYNC;
bool Mute = DEFAULT_MUTE, ForcedMute = DEFAULT_FORCED_MUTE;
bool Reserved1 = DEFAULT_RESERVED1;
bool Reserved2 = DEFAULT_RESERVED2;
int NumBands = DEFAULT_NUM_BANDS;
float PreEmphasis = DEFAULT_PRE_EMPHASIS;
float PostEmphasis = DEFAULT_POST_EMPHASIS;
float StepBy = DEFAULT_STEP_BY;
float Echo = DEFAULT_ECHO;
float FiltersQFactor = DEFAULT_FILTERS_Q_FACTOR;
float Protection[ALL_NUM_BANDS];
float AttackTime[ALL_NUM_BANDS];
float ReleaseTime[ALL_NUM_BANDS];
float Gain[ALL_NUM_BANDS];
float Equalizer[MAX_NUM_BANDS];
float Equalizer_Linear[MAX_NUM_BANDS];
float Post_Emphasis_Linear[MAX_NUM_BANDS];
float Clipper_Linear_p = 0.0f;
float Clipper_Linear_n = 0.0f;

AudioDriver i2sCodec;
DualCompressor comp_st[MAX_NUM_BANDS];
DualCompressor limiter_st;
YummyDSP dspBAND[MAX_NUM_BANDS], dspDelay, dspFmFilter;
FilterNode LPF_BAND[MAX_NUM_BANDS], HPF_BAND[MAX_NUM_BANDS];
FilterNode LPF_FM_FILTER[NumFmFilters];
DelayNode ECHO;

float InputLevelLinear = 1.0f;
float OutputLevelLinearL = 0.0f;
float OutputLevelLinearR = 0.0f;
float BalanceLinearL = 0.0f;
float BalanceLinearR = 0.0f;
float VolCompensationLinear = 1.0f;
float Band_R[MAX_NUM_BANDS];
float Band_L[MAX_NUM_BANDS];
float inputSampleR = 0, inputSampleL = 0, outputSampleR = 0, outputSampleL = 0;

portMUX_TYPE wsMux = portMUX_INITIALIZER_UNLOCKED;
bool wsStateChanged = false;

static float chVuPeak[NUM_MIX_CH] = {0};
static float busVuPeak[NUM_BUSES] = {0};
static float masterVuPeakL = 0, masterVuPeakR = 0;
static uint32_t vuCounter = 0;

void initMixerChannels() {
  for (int ch = 0; ch < NUM_MIX_CH; ch++) {
    for (int b = 0; b < NUM_EQ_BANDS; b++) {
      channels[ch].eq[b] = 0.0f;
    }
    channels[ch].send.lr = (ch < 2) ? 1.0f : 0.8f;
    channels[ch].send.group1 = 0.0f;
    channels[ch].send.group2 = 0.0f;
    channels[ch].send.aux1 = 0.0f;
    channels[ch].send.aux2 = 0.0f;
    channels[ch].reverb = 0.0f;
    channels[ch].comp.ratio = 2.0f;
    channels[ch].comp.attack = 10.0f;
    channels[ch].comp.release = 200.0f;
    channels[ch].comp.threshold = -20.0f;
    channels[ch].comp.gain = 0.0f;
    channels[ch].vu = 0.0f;
    channels[ch].mute = false;
  }
}

float getPeak(float sample) {
  return fabs(sample);
}

void updateVuBroadcast() {
  vuCounter++;
  if (vuCounter < 10) return;
  vuCounter = 0;

  taskENTER_CRITICAL(&wsMux);
  for (int ch = 0; ch < NUM_MIX_CH; ch++) {
    channels[ch].vu = chVuPeak[ch];
    chVuPeak[ch] = 0;
  }
  master.vuMasterL = masterVuPeakL;
  master.vuMasterR = masterVuPeakR;
  master.vuGroup1 = busVuPeak[BUS_GROUP1];
  master.vuGroup2 = busVuPeak[BUS_GROUP2];
  master.vuAux1 = busVuPeak[BUS_AUX1];
  master.vuAux2 = busVuPeak[BUS_AUX2];
  masterVuPeakL = 0;
  masterVuPeakR = 0;
  for (int b = 0; b < NUM_BUSES; b++) busVuPeak[b] = 0;
  wsStateChanged = true;
  taskEXIT_CRITICAL(&wsMux);
}

void changeEqualization() {
  float G = 0.0, lastG = 0.0;
  int center_freq = 0;
  for (int i = 0; i < NumBands; i++) {
    center_freq = getBandCenterFrequency(i);
    G = Equalizer[i] + getEqEmphasis(center_freq, PreEmphasis);
    if (G > lastG) lastG = G;
  }
  for (int i = 0; i < NumBands; i++) {
    center_freq = getBandCenterFrequency(i);
    Equalizer_Linear[i] = decibel_2_linear(Equalizer[i] + getEqEmphasis(center_freq, PreEmphasis) - lastG);
  }
  for (int i = 0; i < NumBands; i++) {
    center_freq = getBandCenterFrequency(i);
    Post_Emphasis_Linear[i] = decibel_2_linear(getEqEmphasis(center_freq, PostEmphasis));
  }
}

void updateFilters() {
  static int num_bands = NumBands;
  if (num_bands != NumBands) {
    num_bands = NumBands;
    ForcedMute = true;
    MuteTimeX100ms = 10;
    for (int i = 0; i < MAX_NUM_BANDS; i++) dspBAND[i].clear();
    for (int i = NumBands - 1; i >= 0; i--) {
      int hpf = FILTER_FREQ[NumBands - 1][i];
      int lpf = FILTER_FREQ[NumBands - 1][i + 1];
      LPF_BAND[i].resetFilter(FilterNode::LPF, lpf, FiltersQFactor);
      HPF_BAND[i].resetFilter(FilterNode::HPF, hpf, FiltersQFactor);
    }
    for (int i = 0; i < NumBands; i++) {
      dspBAND[i].addNode(&LPF_BAND[i]);
      dspBAND[i].addNode(&HPF_BAND[i]);
    }
  }
}

void setAudioVolume() {
  OutputLevelLinearL = decibel_2_linear(OutputLevel + DEFAULT_VOL_CORRECTION);
  OutputLevelLinearR = decibel_2_linear(OutputLevel + DEFAULT_VOL_CORRECTION);
}

void commitConfig() {
  for (int i = 0; i < MAX_NUM_BANDS; i++) {
    int j = MAX_NUM_BANDS;
    comp_st[i].setParams(Protection[j], AttackTime[j], ReleaseTime[j], Gain[j], (float)SampleRateFreq, StepBy);
  }
  int k = MAX_NUM_BANDS + 1;
  limiter_st.setParams(Protection[k], AttackTime[k], ReleaseTime[k], Gain[k], (float)SampleRateFreq, StepBy);
  updateFilters();
  ECHO.setMix(Echo, true);
  changeEqualization();
  InputLevelLinear = decibel_2_linear(InputLevel);
  for (int ch = 0; ch < NUM_PHYS_IN; ch++) {
    inTrimLinear[ch] = decibel_2_linear(inTrim[ch]);
  }
  float P = Balance / 100.0f;
  if (Balance < 50.0f) {
    BalanceLinearL = Balance * 0.02f;
    BalanceLinearR = 1.0f;
  } else if (Balance > 50.0f) {
    BalanceLinearL = 1.0f;
    BalanceLinearR = abs(Balance - 100.0f) * 0.02f;
  } else {
    BalanceLinearL = 1.0f;
    BalanceLinearR = 1.0f;
  }
  setAudioVolume();
  OutputLevelLinearL *= BalanceLinearL;
  OutputLevelLinearR *= BalanceLinearR;
  Clipper_Linear_p = decibel_2_linear(Clipper + 10.0f) * COMP_LIMIT;
  Clipper_Linear_n = Clipper_Linear_p * -1.0f;
  VolCompensationLinear = decibel_2_linear(DEFAULT_VOL_COMPENSATION) + 1.0f;
}

void init_filters() {
  for (int i = 0; i < MAX_NUM_BANDS; i++) {
    dspBAND[i].begin(SampleRateFreq);
    LPF_BAND[i].begin(SampleRateFreq, 2);
    HPF_BAND[i].begin(SampleRateFreq, 2);
  }
  for (int i = NumBands - 1; i >= 0; i--) {
    int hpf = FILTER_FREQ[NumBands - 1][i];
    int lpf = FILTER_FREQ[NumBands - 1][i + 1];
    LPF_BAND[i].setupFilter(FilterNode::LPF, lpf, FiltersQFactor);
    HPF_BAND[i].setupFilter(FilterNode::HPF, hpf, FiltersQFactor);
  }
  for (int i = 0; i < NumBands; i++) {
    dspBAND[i].addNode(&LPF_BAND[i]);
    dspBAND[i].addNode(&HPF_BAND[i]);
  }
  dspFmFilter.begin(SampleRateFreq);
  for (int i = 0; i < NumFmFilters; i++) {
    LPF_FM_FILTER[i].begin(SampleRateFreq, 2);
    LPF_FM_FILTER[i].setupFilter(FilterNode::LPF, 18000, 0.8f);
    dspFmFilter.addNode(&LPF_FM_FILTER[i]);
  }
  dspDelay.begin(SampleRateFreq);
  ECHO.begin(SampleRateFreq, 2);
  ECHO.setDelayMs(30.0f);
  ECHO.setMix(Echo, true);
  dspDelay.addNode(&ECHO);
}

void init_config() {
  Serial.print("\nMounting File System: ");
  if (SPIFFS.begin(false)) {
    Serial.println("OK");
    isGoodFileSystem = true;
  } else {
    Serial.println("FAILED! Formatting...");
    if (SPIFFS.format()) {
      Serial.println("Format OK, remounting...");
      if (SPIFFS.begin(false)) {
        Serial.println("Mounted successfully!");
        isGoodFileSystem = true;
      } else {
        Serial.println("FAILED!");
        isGoodFileSystem = false;
      }
    } else {
      Serial.println("Format failed!");
      isGoodFileSystem = false;
    }
  }
}

String readFile(String path) {
  String S = "";
  if (SPIFFS.exists(path)) {
    File f = SPIFFS.open(path, "r");
    if (f && f.size()) {
      while (f.available()) S += char(f.read());
      f.close();
    }
  }
  return S;
}

bool writeFile(String path, String content) {
  File f = SPIFFS.open(path, "w");
  if (f) {
    f.print(content);
    f.close();
    return true;
  }
  return false;
}

void deleteFile(fs::FS &fs, const char *path) {
  if (fs.remove(path)) {
    Serial.printf("Deleted: %s\n", path);
  }
}

void loadDefaultConfig() {
  initMixerChannels();
  master.lrL = 0.8f;
  master.lrR = 0.8f;
  master.group1 = 0.8f;
  master.group2 = 0.8f;
  master.aux1 = 0.8f;
  master.aux2 = 0.8f;
  master.vuMasterL = 0; master.vuMasterR = 0;
  master.vuGroup1 = 0; master.vuGroup2 = 0;
  master.vuAux1 = 0; master.vuAux2 = 0;

  InputLevel = DEFAULT_INPUT_LEVEL;
  OutputLevel = DEFAULT_OUTPUT_LEVEL;
  Clipper = DEFAULT_CLIPPER;
  Compressor = DEFAULT_COMPRESSOR;
  BandSync = DEFAULT_BAND_SYNC;
  Mute = DEFAULT_MUTE;
  Reserved1 = DEFAULT_RESERVED1;
  Reserved2 = DEFAULT_RESERVED2;
  NumBands = DEFAULT_NUM_BANDS;
  PreEmphasis = DEFAULT_PRE_EMPHASIS;
  PostEmphasis = DEFAULT_POST_EMPHASIS;
  StepBy = DEFAULT_STEP_BY;
  Echo = DEFAULT_ECHO;
  FiltersQFactor = DEFAULT_FILTERS_Q_FACTOR;
  TimeZone = -3;
  for (int i = 0; i < ALL_NUM_BANDS; i++) {
    if (i < MAX_NUM_BANDS) Equalizer[i] = DEFAULT_EQ_BAND;
    Protection[i] = DEFAULT_PROTECTION;
    Gain[i] = DEFAULT_GAIN;
    AttackTime[i] = DEFAULT_ATTACK_TIME;
    ReleaseTime[i] = DEFAULT_RELEASE_TIME;
  }
  for (int ch = 0; ch < NUM_PHYS_IN; ch++) {
    inTrim[ch] = 0.0f;
    inMute[ch] = false;
  }
}

void readConfig() {
  String strJson = readFile(DEFAULT_CONFIG_FILENAME);
  if (strJson.isEmpty()) {
    loadDefaultConfig();
    return;
  }
  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, strJson);
  if (error) {
    Serial.println("JSON parse error, using defaults");
    loadDefaultConfig();
    return;
  }

  for (int ch = 0; ch < NUM_MIX_CH; ch++) {
    char key[8];
    snprintf(key, sizeof(key), "ch%d", ch);
    JsonObject chObj = doc[key];
    if (chObj.isNull()) continue;
    JsonArray eqArr = chObj["eq"];
    if (!eqArr.isNull()) {
      for (int b = 0; b < NUM_EQ_BANDS && b < eqArr.size(); b++) {
        channels[ch].eq[b] = eqArr[b] | 0.0f;
      }
    }
    channels[ch].send.lr = chObj["s_lr"] | ((ch < 2) ? 1.0f : 0.8f);
    channels[ch].send.group1 = chObj["s_g1"] | 0.0f;
    channels[ch].send.group2 = chObj["s_g2"] | 0.0f;
    channels[ch].send.aux1 = chObj["s_a1"] | 0.0f;
    channels[ch].send.aux2 = chObj["s_a2"] | 0.0f;
    channels[ch].reverb = chObj["rev"] | 0.0f;
    channels[ch].comp.ratio = chObj["c_ratio"] | 2.0f;
    channels[ch].comp.attack = chObj["c_atk"] | 10.0f;
    channels[ch].comp.release = chObj["c_rls"] | 200.0f;
    channels[ch].comp.threshold = chObj["c_thr"] | -20.0f;
    channels[ch].comp.gain = chObj["c_gain"] | 0.0f;
    channels[ch].mute = chObj["mute"] | false;
  }

  JsonObject mst = doc["master"];
  if (!mst.isNull()) {
    master.lrL = mst["lrL"] | 0.8f;
    master.lrR = mst["lrR"] | 0.8f;
    master.group1 = mst["g1"] | 0.8f;
    master.group2 = mst["g2"] | 0.8f;
    master.aux1 = mst["a1"] | 0.8f;
    master.aux2 = mst["a2"] | 0.8f;
  }

  InputLevel = doc["Il"] | DEFAULT_INPUT_LEVEL;
  OutputLevel = doc["Ol"] | DEFAULT_OUTPUT_LEVEL;
  Clipper = doc["Clip"] | DEFAULT_CLIPPER;
  Compressor = doc["Comp"] | DEFAULT_COMPRESSOR;
  BandSync = doc["Sync"] | DEFAULT_BAND_SYNC;
  Mute = doc["Mute"] | DEFAULT_MUTE;
  Reserved1 = doc["Res1"] | DEFAULT_RESERVED1;
  Reserved2 = doc["Res2"] | DEFAULT_RESERVED2;
  NumBands = doc["Nb"] | DEFAULT_NUM_BANDS;
  PreEmphasis = doc["PreE"] | DEFAULT_PRE_EMPHASIS;
  PostEmphasis = doc["PostE"] | DEFAULT_POST_EMPHASIS;
  StepBy = doc["Step"] | DEFAULT_STEP_BY;
  Echo = doc["Echo"] | DEFAULT_ECHO;
  FiltersQFactor = doc["Fqf"] | DEFAULT_FILTERS_Q_FACTOR;
  TimeZone = doc["Tz"] | -3;
  for (int i = 0; i < ALL_NUM_BANDS; i++) {
    if (i < MAX_NUM_BANDS) Equalizer[i] = doc["Eq"][i] | DEFAULT_EQ_BAND;
    Protection[i] = doc["Prot"][i] | DEFAULT_PROTECTION;
    Gain[i] = doc["Gn"][i] | DEFAULT_GAIN;
    AttackTime[i] = doc["Atk"][i] | DEFAULT_ATTACK_TIME;
    ReleaseTime[i] = doc["Rls"][i] | DEFAULT_RELEASE_TIME;
  }
  for (int ch = 0; ch < NUM_PHYS_IN; ch++) {
    inTrim[ch] = doc["InTrim"][ch] | 0.0f;
    inMute[ch] = doc["InMute"][ch] | false;
  }
}

bool saveConfig() {
  DynamicJsonDocument doc(8192);
  for (int ch = 0; ch < NUM_MIX_CH; ch++) {
    char key[8];
    snprintf(key, sizeof(key), "ch%d", ch);
    JsonObject chObj = doc.createNestedObject(key);
    JsonArray eqArr = chObj.createNestedArray("eq");
    for (int b = 0; b < NUM_EQ_BANDS; b++) eqArr.add(channels[ch].eq[b]);
    chObj["s_lr"] = channels[ch].send.lr;
    chObj["s_g1"] = channels[ch].send.group1;
    chObj["s_g2"] = channels[ch].send.group2;
    chObj["s_a1"] = channels[ch].send.aux1;
    chObj["s_a2"] = channels[ch].send.aux2;
    chObj["rev"] = channels[ch].reverb;
    chObj["c_ratio"] = channels[ch].comp.ratio;
    chObj["c_atk"] = channels[ch].comp.attack;
    chObj["c_rls"] = channels[ch].comp.release;
    chObj["c_thr"] = channels[ch].comp.threshold;
    chObj["c_gain"] = channels[ch].comp.gain;
    chObj["mute"] = channels[ch].mute;
  }
  JsonObject mst = doc.createNestedObject("master");
  mst["lrL"] = master.lrL; mst["lrR"] = master.lrR;
  mst["g1"] = master.group1; mst["g2"] = master.group2;
  mst["a1"] = master.aux1; mst["a2"] = master.aux2;

  doc["Il"] = InputLevel; doc["Ol"] = OutputLevel;
  doc["Clip"] = Clipper; doc["Comp"] = Compressor;
  doc["Sync"] = BandSync; doc["Mute"] = Mute;
  doc["Res1"] = Reserved1; doc["Res2"] = Reserved2;
  doc["Nb"] = NumBands; doc["PreE"] = PreEmphasis;
  doc["PostE"] = PostEmphasis; doc["Step"] = StepBy;
  doc["Echo"] = Echo; doc["Fqf"] = FiltersQFactor;
  doc["Tz"] = TimeZone;
  JsonArray Eq = doc.createNestedArray("Eq");
  JsonArray Prot = doc.createNestedArray("Prot");
  JsonArray Gn = doc.createNestedArray("Gn");
  JsonArray Atk = doc.createNestedArray("Atk");
  JsonArray Rls = doc.createNestedArray("Rls");
  for (int i = 0; i < ALL_NUM_BANDS; i++) {
    if (i < MAX_NUM_BANDS) Eq.add(Equalizer[i]);
    Prot.add(Protection[i]); Gn.add(Gain[i]);
    Atk.add(AttackTime[i]); Rls.add(ReleaseTime[i]);
  }
  JsonArray inTrimArr = doc.createNestedArray("InTrim");
  JsonArray inMuteArr = doc.createNestedArray("InMute");
  for (int ch = 0; ch < NUM_PHYS_IN; ch++) {
    inTrimArr.add(inTrim[ch]); inMuteArr.add(inMute[ch]);
  }

  String output;
  serializeJson(doc, output);
  if (writeFile(DEFAULT_CONFIG_FILENAME, output)) {
    Serial.println("Config saved!");
    return true;
  } else {
    Serial.println("Config save FAILED!");
    return false;
  }
}

static inline void applyCompressor(float &sample, const CompressorParams &comp, float &gainState) {
  float absS = fabs(sample);
  float threshLin = decibel_2_linear(comp.threshold);
  if (absS > threshLin && comp.ratio > 1.0f) {
    float dbIn = linear_2_decibel(absS);
    float dbOut = comp.threshold + (dbIn - comp.threshold) / comp.ratio;
    float targetGain = decibel_2_linear(dbOut - dbIn);
    float makeup = decibel_2_linear(comp.gain);
    gainState += (targetGain * makeup - gainState) * 0.1f;
    sample *= gainState;
  }
}

void audioTask(void *pvParameters) {
  esp_task_wdt_add(NULL);
  float compGainState[NUM_MIX_CH] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

  while (true) {
    i2sCodec.readBlock();

    for (int i = 0; i < AudioDriver::BufferSize; i++) {
      float physInL = i2sCodec.readSample(i, 0);
      float physInR = i2sCodec.readSample(i, 1);

      physInL *= inTrimLinear[0] * InputLevelLinear;
      physInR *= inTrimLinear[1] * InputLevelLinear;
      if (inMute[0]) physInL = 0;
      if (inMute[1]) physInR = 0;

      float mixSamples[NUM_MIX_CH] = {0};
      mixSamples[0] = physInL;
      mixSamples[1] = physInR;

      float busSums[NUM_BUSES] = {0, 0, 0, 0, 0};

      for (int ch = 0; ch < NUM_MIX_CH; ch++) {
        float s = mixSamples[ch];
        if (channels[ch].mute) s = 0;

        applyCompressor(s, channels[ch].comp, compGainState[ch]);

        float revSig = s;
        if (channels[ch].reverb > 0.01f) {
          revSig = s * 0.3f;
          s = s * (1.0f - channels[ch].reverb * 0.5f) + revSig * channels[ch].reverb * 0.5f;
        }

        busSums[BUS_LR] += s * channels[ch].send.lr;
        busSums[BUS_GROUP1] += s * channels[ch].send.group1;
        busSums[BUS_GROUP2] += s * channels[ch].send.group2;
        busSums[BUS_AUX1] += s * channels[ch].send.aux1;
        busSums[BUS_AUX2] += s * channels[ch].send.aux2;

        float peak = getPeak(s);
        if (peak > chVuPeak[ch]) chVuPeak[ch] = peak;
      }

      // Main L/R bus multiband DSP
      float mainL = 0, mainR = 0;
      inputSampleR = busSums[BUS_LR] * master.lrL * 0.5f;
      inputSampleL = busSums[BUS_LR] * master.lrR * 0.5f;

      if (Compressor) {
        for (int j = 0; j < NumBands; j++) {
          Band_R[j] = dspBAND[j].process(inputSampleR, 0);
          Band_L[j] = dspBAND[j].process(inputSampleL, 1);
          if (Band_R[j] > VuArrValue[j]) VuArrValue[j] = Band_R[j];
          Band_R[j] *= Equalizer_Linear[j];
          Band_L[j] *= Equalizer_Linear[j];
          comp_st[j].process(Band_L[j], Band_R[j]);
          Band_R[j] *= Post_Emphasis_Linear[j];
          Band_L[j] *= Post_Emphasis_Linear[j];
          mainR += Band_R[j];
          mainL += Band_L[j];
        }
        if (NumFmFilters > 0) {
          mainR = dspFmFilter.process(mainR, 0);
          mainL = dspFmFilter.process(mainL, 1);
        }
        if (Echo > 0.0f) {
          mainR = dspDelay.process(mainR, 0);
          mainL = dspDelay.process(mainL, 1);
        }
      } else {
        mainR = inputSampleR;
        mainL = inputSampleL;
        mainR *= VolCompensationLinear;
        mainL *= VolCompensationLinear;
      }
      if (Reserved1) limiter_st.process(mainL, mainR);
      mainR = constrain(mainR, Clipper_Linear_n, Clipper_Linear_p);
      mainL = constrain(mainL, Clipper_Linear_n, Clipper_Linear_p);

      if (Mute || ForcedMute) {
        mainR = 0.f; mainL = 0.f;
      } else {
        mainR *= OutputLevelLinearR;
        mainL *= OutputLevelLinearL;
      }

      float pL = getPeak(mainL), pR = getPeak(mainR);
      if (pL > masterVuPeakL) masterVuPeakL = pL;
      if (pR > masterVuPeakR) masterVuPeakR = pR;
      float pg1 = getPeak(busSums[BUS_GROUP1]);
      float pg2 = getPeak(busSums[BUS_GROUP2]);
      if (pg1 > busVuPeak[BUS_GROUP1]) busVuPeak[BUS_GROUP1] = pg1;
      if (pg2 > busVuPeak[BUS_GROUP2]) busVuPeak[BUS_GROUP2] = pg2;
      float pa1 = getPeak(busSums[BUS_AUX1]);
      float pa2 = getPeak(busSums[BUS_AUX2]);
      if (pa1 > busVuPeak[BUS_AUX1]) busVuPeak[BUS_AUX1] = pa1;
      if (pa2 > busVuPeak[BUS_AUX2]) busVuPeak[BUS_AUX2] = pa2;

      i2sCodec.writeStereoSample(mainR, mainL, i);
    }

    i2sCodec.writeBlock();
    updateVuBroadcast();
    esp_task_wdt_reset();
    vTaskDelay(1);
  }
}

void serialCommTask(void *pvParameters) {
  esp_task_wdt_add(NULL);
  const TickType_t xDelay = 10 / portTICK_PERIOD_MS;
  while (true) {
    commandInterpreter();
    esp_task_wdt_reset();
    vTaskDelay(xDelay);
  }
}
