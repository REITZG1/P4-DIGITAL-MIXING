#pragma once
#include <Arduino.h>
#include "global_vars.h"

const int SEP_COUNT = 60;

void showStatus() {
  Serial.println("\n============================================================");
  Serial.println("SYSTEM STATUS - 8CH DIGITAL MIXER");
  Serial.println("============================================================");
  Serial.print("Sample Rate: "); Serial.print(SampleRateFreq); Serial.println(" Hz");
  Serial.print("Mix Channels: "); Serial.println(NUM_MIX_CH);
  Serial.print("EQ Bands: "); Serial.println(NUM_EQ_BANDS);
  Serial.print("Mute: "); Serial.println(Mute ? "YES" : "NO");
  Serial.print("Main Bus Comp: "); Serial.println(Compressor ? "ON" : "OFF");
  Serial.println("\nMaster Levels:");
  Serial.printf("  L/R: L=%.2f R=%.2f\n", master.lrL, master.lrR);
  Serial.printf("  Group1=%.2f Group2=%.2f\n", master.group1, master.group2);
  Serial.printf("  Aux1=%.2f Aux2=%.2f\n", master.aux1, master.aux2);
  Serial.println("\nChannels:");
  for (int ch = 0; ch < NUM_MIX_CH; ch++) {
    Serial.printf("  CH%d: send LR=%.2f G1=%.2f G2=%.2f A1=%.2f A2=%.2f rev=%.2f %s\n",
      ch+1,
      channels[ch].send.lr, channels[ch].send.group1, channels[ch].send.group2,
      channels[ch].send.aux1, channels[ch].send.aux2, channels[ch].reverb,
      channels[ch].mute ? "[MUTED]" : "");
  }
  Serial.println("============================================================");
}

void commandInterpreter() {
  static String lastS = "";

  if (!Serial.available()) return;
  String text = Serial.readStringUntil('\n');
  text.trim();
  if (text.length() == 0) return;

  String S = text;
  if (!lastS.equals("")) S = lastS;

  if (text.equalsIgnoreCase("menu") || text.equalsIgnoreCase("help")) {
    lastS = "";
    Serial.println("\n============================================================");
    Serial.println("8-CH DIGITAL AUDIO MIXER COMMANDS");
    Serial.println("============================================================");
    Serial.println("  0        Status");
    Serial.println("  1        Mute toggle");
    Serial.println("  2        Save config");
    Serial.println("  3        Reset to defaults");
    Serial.println("  4 ch val Set channel send LR (0-1)");
    Serial.println("  5 ch val Set channel send Group1 (0-1)");
    Serial.println("  6 ch val Set channel send Group2 (0-1)");
    Serial.println("  7 ch val Set channel send Aux1 (0-1)");
    Serial.println("  8 ch val Set channel send Aux2 (0-1)");
    Serial.println("  9 ch val Set channel reverb (0-1)");
    Serial.println(" 10 val   Set master LR L (0-1)");
    Serial.println(" 11 val   Set master LR R (0-1)");
    Serial.println(" 12 val   Set master Group1 (0-1)");
    Serial.println(" 13 val   Set master Group2 (0-1)");
    Serial.println(" 14 val   Set master Aux1 (0-1)");
    Serial.println(" 15 val   Set master Aux2 (0-1)");
    Serial.println(" 16 ch     Toggle channel mute");
    Serial.println(" 20       Compressor (main bus) toggle");
    Serial.println("menu     Show this menu");
    Serial.println("============================================================\n");
    return;
  }

  if (text.equalsIgnoreCase("status") || text.equals("0")) {
    lastS = "";
    showStatus();
    return;
  }

  int idx = S.toInt();

  if (idx == 1) { Mute = !Mute; lastS = ""; Serial.printf("Mute: %s\n", Mute?"ON":"OFF"); }
  else if (idx == 2) { lastS = ""; saveConfig() ? Serial.println("Saved!") : Serial.println("Save failed!"); }
  else if (idx == 3) { lastS = ""; loadDefaultConfig(); commitConfig(); Serial.println("Defaults loaded!"); }
  else if (idx == 20) { Compressor = !Compressor; lastS = ""; Serial.printf("Compressor: %s\n", Compressor?"ON":"OFF"); }

  // Channel send commands: idx ch val (e.g., "4 1 0.5" = ch1 send LR=0.5)
  else if ((idx >= 4 && idx <= 9) || idx == 16) {
    int s1 = text.indexOf(' ', 2);
    if (s1 > 0) {
      int ch = text.substring(2, s1).toInt() - 1;
      float v = text.substring(s1+1).toFloat();
      if (ch < 0 || ch >= NUM_MIX_CH) { Serial.println("Invalid channel"); lastS = ""; return; }
      switch (idx) {
        case 4: channels[ch].send.lr = constrain(v, 0, 1); break;
        case 5: channels[ch].send.group1 = constrain(v, 0, 1); break;
        case 6: channels[ch].send.group2 = constrain(v, 0, 1); break;
        case 7: channels[ch].send.aux1 = constrain(v, 0, 1); break;
        case 8: channels[ch].send.aux2 = constrain(v, 0, 1); break;
        case 9: channels[ch].reverb = constrain(v, 0, 1); break;
        case 16: channels[ch].mute = !channels[ch].mute; break;
      }
      commitConfig();
      Serial.printf("CH%d updated\n", ch+1);
    }
    lastS = "";
  }

  // Master commands: idx val (e.g., "10 0.75")
  else if (idx >= 10 && idx <= 15) {
    int s1 = text.indexOf(' ', 2);
    if (s1 > 0) {
      float v = text.substring(s1+1).toFloat();
      v = constrain(v, 0, 1);
      switch (idx) {
        case 10: master.lrL = v; break;
        case 11: master.lrR = v; break;
        case 12: master.group1 = v; break;
        case 13: master.group2 = v; break;
        case 14: master.aux1 = v; break;
        case 15: master.aux2 = v; break;
      }
      Serial.printf("Master updated\n");
    }
    lastS = "";
  }

  else {
    Serial.println("Unknown command. Type 'menu' for commands.");
    lastS = "";
  }
}
