#if !defined(ESP32)
  #error This sketch needs an ESP32
#else

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <DS3231.h>
#include <ESPRotary.h>
#include <Preferences.h>
#define LED 2
#define INLET_VALVE 23
#define IGNITION 16
#define SPRINKLERS 19
#define STARTER 18
#define WATER_LOW 5
#define WATER_HIGH 17
#define SOL_INLET 4
#define SOL_SPRINKLERS 0
#define ROT_SW 36
#define ROT_CLK 34
#define ROT_DT 39
#define PUMP_SENSE 35
#define CLICKS_PER_STEP 4

#define TANK_LOW 64
#define TANK_FULL 65
#define FILLING 66

#define INLET_VALVE_ENABLED "InletValveEn"
#define SPRINKLERS_ENABLED "sprValveEn"
#define PUMP_ENABLED "pumpEn"
#define STARTER_RETRY "starterRetry"
#define STARTER_DELAY "starterDelay"
#define CRANK_TIME "crankTime"
#define PUMP_RETRY_DELAY "pumpRetryDelay"

int currentWaterLevel = 0;         // 0=LOW 1=Medium 2=Full
int lastInletTriggerSource = 0;    // 0 = Null, 1 = By water level, 2 = By timer
bool inletTimerStatus = HIGH;      // high is solenoid off
bool sprinklerTimerStatus = HIGH;  // high is solenoid off
int isInSettings = 0;              // display mode
char* currentPumpStatus = 0;
bool updateScreen = false;
bool menuInterrupt = false;
int loopCounter = 0;

LiquidCrystal_PCF8574 lcd(0x27);
DS3231 rtc;
ESPRotary enc;
Preferences preferences;
hw_timer_t *timer1 = NULL;
hw_timer_t *timer2 = NULL;

void IRAM_ATTR handleLoop() {
  enc.loop();
}

void IRAM_ATTR metronome() {
  autoLcdBacklightISR();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial) { delay(100); }
  Serial.println();
  Serial.println("******************************************************");
  Serial.print("Starting up...");
  pinMode(INLET_VALVE, OUTPUT);
  pinMode(IGNITION, OUTPUT);
  pinMode(SPRINKLERS, OUTPUT);
  pinMode(STARTER, OUTPUT);
  pinMode(WATER_LOW, INPUT_PULLUP);
  pinMode(WATER_HIGH, INPUT_PULLUP);
  pinMode(SOL_INLET, INPUT_PULLUP);
  pinMode(SOL_SPRINKLERS, INPUT_PULLUP);
  pinMode(PUMP_SENSE, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(ROT_SW, INPUT);

  attachInterrupt(digitalPinToInterrupt(WATER_LOW), onWaterLow, CHANGE);
  attachInterrupt(digitalPinToInterrupt(WATER_HIGH), onWaterFullChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SOL_INLET), onInletTimer, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SOL_SPRINKLERS), onSprinklerTimer, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROT_SW), onRotSw, FALLING);

  timer1 = timerBegin(0, 80, true);
  timerAttachInterrupt(timer1, &handleLoop, true);
  timerAlarmWrite(timer1, 1000, true); 
  timerAlarmEnable(timer1);

  timer2 = timerBegin(1, 80, true);
  timerAttachInterrupt(timer2, &metronome, true);
  timerAlarmWrite(timer2, 1000000, true);
  timerAlarmEnable(timer2);

  // Start by turning everything off
  digitalWrite(INLET_VALVE, LOW);
  digitalWrite(IGNITION, LOW);
  digitalWrite(SPRINKLERS, LOW);
  digitalWrite(STARTER, LOW);

  enc.begin(ROT_CLK, ROT_DT, CLICKS_PER_STEP);
  // enc.setChangedHandler(rotate);
  // enc.setLeftRotationHandler(showDirection);
  // enc.setRightRotationHandler(showDirection);

  enc.setIncrement(1);

  preferences.begin("settings", false);
  loadParameterValues();
  
  // initTime();
  delay(2000);

  initLCD();
  lcd.clear();
  updateScreen = true;
  displayStatus();
}

void loop() {

  // Serial.println("Settings");
  // for(int i = 0; i < 7; i++)
  // {
  //   Serial.print(i);
  //   Serial.print(" ");
  //   Serial.println(getCurrentVal(i));
  // }


  checkBacklight();
  checkInletValve();
  checkWaterLevel();
  checkSprinklers();
  checkPump();

  menu();
  
  delay(100);
  if(loopCounter < 600){
    // allow time to enter menu before starting routine
    loopCounter++;
    return;
  }
  // Serial.println("Passed loop counter 600");
  if(menuInterrupt){
    return;
  }
  displayStatus();
  // Serial.println("Passed menu interrupt");
  // Only execute the rest if not in menu
  if (currentWaterLevel == 0 && isInletValveOpen()==false) {
    inletValve(HIGH, 0);
    lastInletTriggerSource = 1;
  } else if (currentWaterLevel == 2 && isInletValveOpen() && lastInletTriggerSource == 1) {
    inletValve(LOW, 0);
    lastInletTriggerSource = 0;
  }
  if (currentWaterLevel < 2 && inletTimerStatus == LOW && isInletValveOpen()==false) {
    inletValve(HIGH, 1);
    lastInletTriggerSource = 2;  // set trigger source to timer
  } else if (inletTimerStatus == HIGH && isInletValveOpen() && lastInletTriggerSource == 2) {
    inletValve(LOW, 1);
    lastInletTriggerSource = 0;
  }
  if (sprinklerTimerStatus == LOW && areSprinklersOn()==false) {
    sprinklers(HIGH, 1);
  } else if (sprinklerTimerStatus == HIGH && areSprinklersOn()) {
    sprinklers(LOW, 1);
  }
  displayStatus();
}

#endif