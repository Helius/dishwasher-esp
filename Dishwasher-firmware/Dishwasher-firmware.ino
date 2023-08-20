// кастомные кнопки OTA прошивки

#define AP_SSID ***
#define AP_PASS ***

// подключить библиотеку файловой системы (до #include GyverPortal)
#include <LittleFS.h>

#include <GyverPortal.h>
GyverPortal ui(&LittleFS);  // передать ссылку на fs (SPIFFS/LittleFS)

#include <AceRoutine.h>
using namespace ace_routine;

#include "DishWasher.h"
#include "DIshWasherWM.h"
#include "SoundPlayer.h"

#define InletValveSw "inletsw"
#define DrainSw "drainsw"
#define AidSw "aidsw"
#define CircPumpSw "circpumpsw"
#define HeaterSw "heatersw"
#define ResetBtn "resetbtn"

SoundPlayer snd(26);  // buzzer pin 26

DishWasher dw(snd);
DishWasherVm vm;
LedCoroutine ledC(dw);
TemprLogCoroutine temprC(dw);

class IOCoroutine : public Coroutine {
public:
  int runCoroutine() override {
    COROUTINE_BEGIN();
    while (true) {
      dw.doIO();
      COROUTINE_DELAY(100);
    }
    COROUTINE_END();
  }
};

class UiCoroutine : public Coroutine {
public:
  int runCoroutine() override {
    COROUTINE_BEGIN();
    while (true) {
      vm.update(dw);
      ui.tick();
      COROUTINE_DELAY(500);
    }
    COROUTINE_END();
  }
};


// конструктор страницы
void build() {
  const char *names[] = {
    "temperature"
  };

  GP.BUILD_BEGIN();
  GP.THEME(GP_DARK);

  GP.LABEL("DishWasher v2.2");

  GP.BREAK();
  GP.BREAK();
  GP.LABEL("State: " + vm.stateStr);
  GP.BREAK();
  GP.BREAK();
  GP.LABEL("Door: " + vm.doorStr);
  GP.LABEL("Press: " + vm.pressostatStr);
  GP.LABEL("Salt switch: " + vm.saltSwitchStr);
  GP.BREAK();
  GP.LABEL("Temperature: " + vm.temperatureStr);
  GP.BREAK();
  GP.BREAK();
  GP.LABEL("Fill Water");
  GP.SWITCH(InletValveSw, dw.inletValve);
  GP.LABEL("Drain Water");
  GP.SWITCH(DrainSw, dw.outputs.drainPump);
  GP.LABEL("Throw aid");
  GP.SWITCH(AidSw, dw.outputs.throwAid);
  GP.LABEL("Wash");
  GP.SWITCH(CircPumpSw, dw.outputs.washingPump);
  GP.LABEL("Heater");
  GP.SWITCH(HeaterSw, dw.heater);
  GP.BREAK();
  GP.BREAK();
  GP.PLOT<1, 200>("plot1", names, &temprC.temprLog);
  GP.BREAK();
  //GP.BUTTON(ResetBtn, "Reset");
  GP.BREAK();
  GP.OTA_FIRMWARE();
  GP.BUILD_END();
}

UiCoroutine uiCoroutine;
IOCoroutine iOCoroutine;
FillWaterCoroutine fillWaterCoroutine(dw);

void setup() {
  dw.initGpio();
  startup();

  if (!LittleFS.begin()) Serial.println("FS Error");

  ui.attachBuild(build);
  ui.attach(action);
  ui.start();
  ui.enableOTA();
  CoroutineScheduler::setup();
  snd.play(SoundPlayer::Melody::Start);
}

void action() {
  // был клик по компоненту
  if (ui.click()) {
    // проверяем компоненты и обновляем переменные

    // if (ui.click(ResetBtn)) {
    //   Serial.println("Button click");
    // }
    if (ui.clickBool(InletValveSw, dw.inletValve)) {
    }
    if (ui.clickBool(DrainSw, dw.outputs.drainPump)) {
    }
    if (ui.clickBool(AidSw, dw.outputs.throwAid)) {
    }
    if (ui.clickBool(CircPumpSw, dw.outputs.washingPump)) {
    }
    if (ui.clickBool(HeaterSw, dw.heater)) {
    }
  }
}

void loop() {
  CoroutineScheduler::loop();
}

void startup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(AP_SSID, AP_PASS);
  WiFi.setHostname("DishWasher");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());
}
