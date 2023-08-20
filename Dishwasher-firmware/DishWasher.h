#include "esp32-hal-gpio.h"
#pragma once

#include "ace_routine/Coroutine.h"
#include "SoundPlayer.h"

#define DRAIN_WATER \
  state = State::Drain; \
  COROUTINE_DELAY_SECONDS(60); /* wait to water collect in tray */ \
  while (!pressostate) { \
    outputs.drainPump = true; \
    COROUTINE_DELAY_SECONDS(5); \
  } \
  COROUTINE_DELAY_SECONDS(10); \
  outputs.drainPump = false;


class DishWasher : public Coroutine {

  enum OUT : uint8_t {
    HeaterOut = 4,
    InletValveOut = 13,
    DrainPumpOut = 16,
    WashingPumpOut = 17,
    AidThrowOut = 18,
  };

  enum LED : uint8_t {
    Led00 = 19,  // common (anod)
    Led01 = 21,  // mode (kathod)
    Led02 = 22,  // salt (kathod)
    Led03 = 23,  // star (kathod)
  };

  enum IN : uint8_t {
    DoorIn = 34,
    PressostateIn = 35,
    SaltSensorIn = 27,
    ButtonIn = 32,
  };

  enum AN : uint8_t {
    Ntc = 36
  };


public:
  DishWasher(SoundPlayer& snd)
    : mSnd(snd) {}

public:
  bool onPause = false;

  bool doorOpen = false;
  bool saltSwitch = false;
  bool pressostate = false;
  uint16_t temperature = 0;
  bool buttonPressed = false;

  struct Outputs {
    bool drainPump = false;
    bool washingPump = false;
    bool throwAid = false;
  };

  bool heater = false;
  bool inletValve = false;

  enum class State {
    Idle,
    Armed,
    PreWashing,
    Washing,
    Drain,
    Rinsing,
    Drying,
    Finish,
  };

  State state = State::Idle;
  int cycle = 0;
  Outputs outputs;

public:
  int runCoroutine() override {
    COROUTINE_BEGIN();
    cycle = 0;
    while (true) {
      // if we got some water - drain it
      while (!pressostate) {
        outputs.drainPump = true;
        COROUTINE_DELAY_SECONDS(5);
      }
      outputs.drainPump = false;
      mSnd.play(SoundPlayer::Melody::Start);
      state = State::Idle;

      COROUTINE_AWAIT(buttonPressed);
      state = State::Armed;
      COROUTINE_DELAY_SECONDS(3);
      COROUTINE_AWAIT(!doorOpen);

      state = State::PreWashing;

      for (cycle = 0; cycle < 2; ++cycle) {
        outputs.washingPump = true;
        COROUTINE_DELAY_SECONDS(10 * timeMultiplyer);
        outputs.washingPump = false;
        COROUTINE_DELAY_SECONDS(10 * timeMultiplyer);
      }

      DRAIN_WATER

      state = State::Washing;
      outputs.throwAid = true;

      for (cycle = 0; cycle < 3; ++cycle) {
        outputs.washingPump = true;
        COROUTINE_DELAY_SECONDS(5 * timeMultiplyer);
        outputs.throwAid = false;
        COROUTINE_DELAY_SECONDS(35 * timeMultiplyer);
        outputs.washingPump = false;
        COROUTINE_DELAY_SECONDS(10 * timeMultiplyer);
      }

      DRAIN_WATER

      state = State::Rinsing;
      outputs.washingPump = true;
      COROUTINE_DELAY_SECONDS(30 * timeMultiplyer);
      outputs.washingPump = false;

      DRAIN_WATER
      state = State::Drying;

      mSnd.play(SoundPlayer::Melody::Drying);

      COROUTINE_DELAY_SECONDS(10 * timeMultiplyer);
      DRAIN_WATER
      state = State::Finish;
      mSnd.play(SoundPlayer::Melody::Finish);
    }
    COROUTINE_END();
  }

  void doIO() {
    // get inputs
    doorOpen = digitalRead(IN::DoorIn);
    saltSwitch = digitalRead(IN::SaltSensorIn);
    pressostate = digitalRead(IN::PressostateIn);
    buttonPressed = !digitalRead(IN::ButtonIn);
    // read temperature
    temperature = analogRead(AN::Ntc);

    if (doorOpen && state == State::Drying) {
      state = State::Idle;
      reset();
    }

    if (!onPause && doorOpen && (state != State::Idle && state != State::Armed && state != State::Finish && state != State::Drying)) {
      savedOutputs = outputs;
      suspend();
      outputs = Outputs();
      onPause = true;
    }

    if (!doorOpen && onPause) {
      outputs = savedOutputs;
      onPause = false;
      resume();
    }

    if (!doorOpen && outputs.washingPump && !pressostate) {
      if (temperature > 1450) {
        heater = true;
      }
      if (temperature < 1350) {
        heater = false;
      }
    } else {
      heater = false;
    }

    // update output values
    digitalWrite(OUT::HeaterOut, static_cast<uint8_t>(heater));
    digitalWrite(OUT::InletValveOut, static_cast<uint8_t>(inletValve));
    digitalWrite(OUT::DrainPumpOut, static_cast<uint8_t>(outputs.drainPump));
    digitalWrite(OUT::WashingPumpOut, static_cast<uint8_t>(outputs.washingPump));
    digitalWrite(OUT::AidThrowOut, static_cast<uint8_t>(outputs.throwAid));

    // update leds
    digitalWrite(LED::Led00, 1);
    digitalWrite(LED::Led02, static_cast<uint8_t>(saltSwitch));
  }


  void setupLed() {
    static int ledState = 0;
    if (state == State::Finish && state == State::Drying) {
      digitalWrite(LED::Led01, 0);
    } else if (state == State::Idle) {
      digitalWrite(LED::Led01, 1);
    } else {
      ledState = !ledState;
      digitalWrite(LED::Led01, ledState);
    }
  }

  void fillWater() {
    if (onPause) {
      inletValve = false;
    } else {
      switch (state) {
        case State::PreWashing:
        case State::Washing:
        case State::Rinsing:
          if (pressostate) {
            inletValve = true;
          } else {
            inletValve = false;
          }
          break;
        case State::Idle:
        case State::Armed:
        case State::Drain:
        case State::Drying:
        case State::Finish:
        default:
          inletValve = false;
          break;
      }
    }
  }

  void initGpio() {
    pinMode(OUT::HeaterOut, OUTPUT);
    pinMode(OUT::InletValveOut, OUTPUT);
    pinMode(OUT::DrainPumpOut, OUTPUT);
    pinMode(OUT::WashingPumpOut, OUTPUT);
    pinMode(OUT::AidThrowOut, OUTPUT);

    pinMode(LED::Led00, OUTPUT);
    pinMode(LED::Led01, OUTPUT);
    pinMode(LED::Led02, OUTPUT);
    pinMode(LED::Led03, OUTPUT);

    pinMode(IN::DoorIn, INPUT_PULLUP);
    pinMode(IN::PressostateIn, INPUT_PULLUP);
    pinMode(IN::SaltSensorIn, INPUT_PULLUP);
    pinMode(IN::ButtonIn, INPUT_PULLUP);

    pinMode(AN::Ntc, ANALOG);

    digitalWrite(LED::Led00, 1);
    digitalWrite(LED::Led01, 1);
    digitalWrite(LED::Led02, 1);
    digitalWrite(LED::Led03, 1);
  }


private:
  Outputs savedOutputs;
  int timeMultiplyer = 50;  //'minute' multiplyer
  SoundPlayer& mSnd;
};

class FillWaterCoroutine : public Coroutine {
public:
  FillWaterCoroutine(DishWasher& dw)
    : dw(dw) {}
public:
  int runCoroutine() override {
    COROUTINE_BEGIN();
    while (true) {
      dw.fillWater();
      COROUTINE_DELAY(1000);
    }
    COROUTINE_END();
  }
private:
  DishWasher& dw;
};

class LedCoroutine : public Coroutine {
public:
  LedCoroutine(DishWasher& dw)
    : dw(dw) {}
  int runCoroutine() override {
    COROUTINE_BEGIN();
    while (true) {
      dw.setupLed();
      COROUTINE_DELAY(300);
    }
    COROUTINE_END();
  };
private:
  DishWasher& dw;
};

class TemprLogCoroutine : public Coroutine {
public:
  TemprLogCoroutine(DishWasher& dw)
    : dw(dw) {}
  int runCoroutine() override {
    COROUTINE_BEGIN();
    while (true) {
      if (dw.state != DishWasher::State::Idle
          && dw.state != DishWasher::State::Finish
          && dw.state != DishWasher::State::Armed) {

        temprLog[temprLogIndex++] = dw.temperature;
        if (temprLogIndex >= 200) {
          temprLogIndex = 0;
        }
        COROUTINE_DELAY_SECONDS(60);
      } else {
        COROUTINE_DELAY_SECONDS(5);
      }
    }
    COROUTINE_END();
  };
  int16_t temprLog[200];
private:
  int temprLogIndex = 0;
  DishWasher& dw;
};