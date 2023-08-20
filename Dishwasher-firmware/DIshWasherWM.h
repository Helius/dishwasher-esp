#pragma once

class DishWasherVm {
public:
  String stateStr = "?";
  
  String doorStr = "?";
  String pressostatStr = "?";
  String saltSwitchStr = "?";
  String temperatureStr = "?";

  void update(const DishWasher & dw) {
    stateStr = (dw.onPause ? "[Paused]" : "-> ") + state2Str(dw.state, dw.cycle);

    doorStr = dw.doorOpen ? "Open" : "Close";
    pressostatStr = dw.pressostate ? "Empty" : "Full";
    saltSwitchStr = dw.saltSwitch ? "OFF" : "ON";
    temperatureStr = String(dw.temperature);
    Serial.println("--------------");
    Serial.println("state: " + stateStr);
    Serial.print("door: " + String(dw.doorOpen));
    Serial.print(", pres: " + String(dw.pressostate));
    Serial.print(", salt: " + String(dw.saltSwitch));
    Serial.println(", butn: " + String(dw.buttonPressed));
    Serial.println("tempr: " + String(dw.temperature));
    
    Serial.print(WiFi.RSSI()); Serial.print(", "); Serial.println(WiFi.localIP());
    Serial.println("");
  }

  private:
  String state2Str(DishWasher::State state, int cycle) {
    switch(state) {
      case DishWasher::State::Idle:
        return "Idle";
      case DishWasher::State::Armed:
        return "Armed";
      case DishWasher::State::PreWashing:
        return String("Pre Washing x ") + String(cycle);
      case DishWasher::State::Washing:
        return String("Washing x ") + String(cycle);
      case DishWasher::State::Drain:
        return "Drain water";
      case DishWasher::State::Rinsing:
        return "Rinsing";
      case DishWasher::State::Drying:
        return "Drying";
      case DishWasher::State::Finish:
        return "Finished";
      default:
        return "?";
    }
  }
};