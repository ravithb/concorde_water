#if !defined(ESP32)
  #error This sketch needs an ESP32
#else

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <DS3231.h>
#include <ESPRotary.h>
#include <Preferences.h>
#include <Adafruit_PWMServoDriver.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define LED 2
#define INLET_VALVE 23
#define IGNITION 16
#define SPRINKLERS 19
#define STARTER 18
#define WATER_LOW 17
#define WATER_HIGH 5
#define SOL_INLET 4
#define SOL_SPRINKLERS 0
#define ROT_SW 34
#define ROT_CLK 33
#define ROT_DT 35
#define PUMP_SENSE 32
#define CLICKS_PER_STEP 4
#define SERVO_DECOMP 2
#define SERVO_THROTTLE 3

#define TANK_LOW 64
#define TANK_FULL 65
#define FILLING 66

#define INLET_VALVE_ENABLED "InletValveEn"
#define SPRINKLERS_ENABLED "sprValveEn"
#define PUMP_ENABLED "pumpEn"
#define STARTER_RETRY "starterRetry"
#define STARTER_DELAY "starterDelay"
#define CRANK_TIME "crankTime"
#define PUMP_SENSE_DELAY "pumpSenseDelay"
#define PUMP_RETRY_DELAY "pumpRetryDelay"
#define DECOMP_LEVER_DELAY "decomprLeverDelay"
#define DECOMP_LEVER_OFF "decompLeverOff"
#define DECOMP_LEVER_ON "decompLeverOn"
#define THROTTLE_STOP "throttleStop"
#define THROTTLE_RUN "throttleRun"
#define THROTTLE_START "throttleStart"
#define SERVOMIN  175 // This is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  540 // This is the 'maximum' pulse length count (out of 4096)
#define USMIN  600 // This is the rounded 'minimum' microsecond length based on the minimum pulse of 150
#define USMAX  2400 // This is the rounded 'maximum' microsecond length based on the maximum pulse of 600
#define SERVO_FREQ 60 // Analog servos run at ~50 Hz updates
#define MECH_VALVE_DELAY 32000 // Time taken for the mechanical valve to open or close fully in milliseconds

#define BLE_SERVICE_UUID                  "c0ae3000-da8a-45c6-a5c5-fbad540c2bf3"
#define BLE_CHARACTEREISTIC_STATUS_UUID   "c0ae3001-da8a-45c6-a5c5-fbad540c2bf3"
#define BLE_CHARACTEREISTIC_LOGS_UUID     "c0ae3002-da8a-45c6-a5c5-fbad540c2bf3"
#define BLE_CHARACTEREISTIC_COMMANDS_UUID "c0ae3003-da8a-45c6-a5c5-fbad540c2bf3"
#define BT_DEVICE_NAME "CMFC_IRRIGATION"

int currentWaterLevel = 0;         // 0=LOW 1=Medium 2=Full
int lastInletTriggerSource = 0;    // 0 = Null, 1 = By water level, 2 = By timer
bool inletTimerStatus = HIGH;      // high is solenoid off
bool sprinklerTimerStatus = HIGH;  // high is solenoid off
bool isInMenu = false;              // display mode
char* currentPumpStatus = 0;
bool updateScreen = false;
bool menuInterrupt = false;
int loopCounter = 0;
bool hasLCD = false;
int manualWatering = 0;
int manualFilling = 0;
int pumpTest = 0;
TaskHandle_t hwLoopTask;
bool hwBusy = false;
int selectedActiveOption = -1;
int selectedActiveOptParam = -1;
bool runActiveOptionFlag = false;
unsigned long lastBtUpdate = 0;

LiquidCrystal_PCF8574 lcd(0x27);
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x44);
// DS3231 rtc;
ESPRotary enc;
Preferences preferences;
hw_timer_t *timer1 = NULL;
hw_timer_t *timer2 = NULL;

void debugLog(String s);

void IRAM_ATTR handleLoop() {
  enc.loop();
}

void IRAM_ATTR metronome() {
  autoLcdBacklightISR();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin();
  // Wire.setTimeout(20);
  while (!Serial) { delay(100); }
  
  bleSetup();


  Serial.println();
  debugLog("******************************************************");
  debugLog("Starting up...");
  pinMode(INLET_VALVE, OUTPUT);
  pinMode(IGNITION, OUTPUT);
  pinMode(SPRINKLERS, OUTPUT);
  pinMode(STARTER, OUTPUT);
  pinMode(WATER_LOW, INPUT);
  pinMode(WATER_HIGH, INPUT);
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

  timer1 = timerBegin(1000000);
  timerAttachInterrupt(timer1, &handleLoop);
  timerAlarm(timer1, 1000, true, 0); 

  timer2 = timerBegin(1000000);
  timerAttachInterrupt(timer2, &metronome);
  timerAlarm(timer2, 1000000, true, 0);

  // Start by turning everything off
  digitalWrite(INLET_VALVE, LOW);
  digitalWrite(IGNITION, LOW);
  digitalWrite(SPRINKLERS, LOW);
  digitalWrite(STARTER, LOW);

  enc.begin(ROT_CLK, ROT_DT, CLICKS_PER_STEP);

  enc.setIncrement(1);

  preferences.begin("settings", false);
  loadParameterValues();
  
  // initTime();
  delay(2000);

  initLCD();
  if(hasLCD){
    lcd.clear();
  }
  updateScreen = true;
  displayStatus();


  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates

  xTaskCreatePinnedToCore(
                  hwLoop,   /* Task function. */
                  "HWLoop",     /* name of task. */
                  10000,       /* Stack size of task */
                  NULL,        /* parameter of the task */
                  1,           /* priority of the task */
                  &hwLoopTask,      /* Task handle to keep track of created task */
                  0);          /* pin task to core 0 */
  delay(500); 

  // 
  debugLog("Setup complete");
}

void loop() {

  checkBacklight();
  // while busy there are extra messages that get updated by the hwLoop
  if(!hwBusy){
    checkInletValve();
    checkWaterLevel();
    checkSprinklers();
    checkPump();
  }

  menu();
  
  delay(100);
  if(loopCounter < 100){
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
}

// hardware control loop
void hwLoop(void* pvParameters) {
  while(true){
    if(manualFilling <= 0){
      if (currentWaterLevel == 0 && isInletValveOpen()==false) {
        inletValve(HIGH, 0);
        lastInletTriggerSource = 1;
      } else if (currentWaterLevel == 2 && isInletValveOpen()) {
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
    }else{
      // Serial.print("Manual filling ");
      // Serial.print(manualFilling);
      // Serial.print(isInletValveOpen()?" open ":" closed ");
      // Serial.println(isActiveOptionSet()?" opt param set ":" opt param null");
      // manual filling ignores the water level sensor
      if (manualFilling == 2 && isInletValveOpen()==false && isActiveOptionSet()) {
        inletValve(HIGH, 2);
        lastInletTriggerSource = 3;  // set trigger source to manual
      } else if (manualFilling == 1 && isInletValveOpen() && isActiveOptionSet()) {
        inletValve(LOW, 2);
        lastInletTriggerSource = 3;
      }
    }
    if (sprinklerTimerStatus == LOW && areSprinklersOn()==false) {
      sprinklers(HIGH, 1);
    } else if (sprinklerTimerStatus == HIGH && areSprinklersOn()) {
      sprinklers(LOW, 1);
    }
    if(isActiveOptionSet()) {
      // Serial.println("if isActiveOptionSet");
      // Serial.println(selectedActiveOption);
      // Serial.println(selectedActiveOptParam);
      resetSelectedActiveOption();
      if (manualWatering == 2 && areSprinklersOn()==false) {
        sprinklers(HIGH, 2);
        manualWatering = 0; // reset the action
      } else if (manualWatering == 1 && areSprinklersOn()) {
        sprinklers(LOW, 2);
        manualWatering = 0; // reset the action
      }

      if(isPumpStarted() && pumpTest == 1){
        stopPump();
        pumpTest = 0;
      }else if(isPumpStarted()==false && pumpTest==2){
        startPump(false);
        pumpTest = 0;
      }
    }
    
    delay(100);
  }
  
}


#endif