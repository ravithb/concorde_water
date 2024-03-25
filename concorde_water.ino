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
#define LED 2
#define INLET_VALVE 23
#define IGNITION 16
#define SPRINKLERS 19
#define STARTER 18
#define WATER_LOW 17
#define WATER_HIGH 5
#define SOL_INLET 4
#define SOL_SPRINKLERS 0
#define ROT_SW 36
#define ROT_CLK 34
#define ROT_DT 39
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
#define PUMP_RETRY_DELAY "pumpRetryDelay"
#define DECOMPR_LEVER_DELAY "decomprLeverDelay"
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
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x44);

// DS3231 rtc;
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
  Wire.begin();
  while (!Serial) { delay(100); }
  Serial.println();
  Serial.println("******************************************************");
  Serial.print("Starting up...");
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


  pwm.begin();
  //pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates

}

void loop() {

  // for( int angle =0; angle<121; angle +=2){
  //   if(angle > 60){
  //     pwm.setPWM(3, 0, angleToPulse(0) );
  //   }else{
  //     pwm.setPWM(3, 0, angleToPulse(120) );
  //   }
  //   delay(100);
  // }



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