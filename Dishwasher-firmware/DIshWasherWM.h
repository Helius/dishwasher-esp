#include "DishWasher.h"
#pragma once

class DishWasherVm {
public:
  String stateStr = "?";
  String timeStr = "?";

  String doorStr = "?";
  String pressostatStr = "?";
  String saltSwitchStr = "?";
  String temperatureStr = "?";

  void update(const DishWasher& dw) {
    stateStr = (dw.onPause ? "[Paused]" : "-> ") + state2Str(dw.state, dw);

    doorStr = dw.doorOpen ? "Open" : "Close";
    pressostatStr = dw.pressostate ? "Empty" : "Full";
    saltSwitchStr = dw.saltSwitch ? "OFF" : "ON";
    temperatureStr = String(dw.temperature);
    timeStr = secToTimeStr(millis()/1000 - dw.startSec) + " of " + secToTimeStr(dw.totalTime()) + " (" + (millis()/1000 - dw.startSec)*100/(dw.totalTime()+1) + "%)";
    
    Serial.println("--------------");
    Serial.println("state: " + stateStr);
    Serial.print("door: " + String(dw.doorOpen));
    Serial.print(", pres: " + String(dw.pressostate));
    Serial.print(", salt: " + String(dw.saltSwitch));
    Serial.println(", butn: " + String(dw.buttonPressed));
    Serial.println("tempr: " + String(dw.temperature));

    Serial.print(WiFi.RSSI());
    Serial.print(", ");
    Serial.println(WiFi.localIP());
    Serial.println("");
  }

private:

  String secToTimeStr(int sec) {
    int h = sec/3600;
    int m = (sec - h*3600)/60;
    int s = sec - h*3600 - m*60; 
    return String(h) + ":" + String(m) + ":" + String(s);
  }

  String state2Str(DishWasher::State state, const DishWasher & dw) {
    switch (state) {
      case DishWasher::State::Idle:
        return "Idle";
      case DishWasher::State::Armed:
        return "Armed";
      case DishWasher::State::PreWashing:
        return String("Pre Washing: ") + String(dw.cycle + 1) + String(" of ") + String(dw.prewashCycles);
      case DishWasher::State::Washing:
        return String("Washing: ") + String(dw.cycle + 1) + " of " + String(dw.cycles);
      case DishWasher::State::Drain:
        return "Drain water";
      case DishWasher::State::Rinsing:
        return String("Rinsing:") + String(dw.cycle) + String(" of ") + String(dw.rinsingCycles);
      case DishWasher::State::Drying:
        return "Drying";
      case DishWasher::State::Finish:
        return "Finished";
      default:
        return "?";
    }
  }
};