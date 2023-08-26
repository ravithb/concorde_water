#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

#define INLET_VALVE 23
#define IGNITION 16
#define SPRINKLERS 19
#define STARTER 18
#define WATER_LOW 5
#define WATER_HIGH 17
#define SOL_INLET 4
#define SOL_SPRINKLERS 0
#define ROT_SW 27
#define ROT_CLK 14
#define ROT_DT 12

#define TANK_LOW 64
#define TANK_FULL 65
#define FILLING 66

int currentWaterLevel = 0; // 0=LOW 1=Medium 2=Full
int lastInletTriggerSource = 0; // 0 = Null, 1 = By water level, 2 = By timer
bool inletTimerStatus = HIGH; // high is solenoid off
bool sprinklerTimerStatus = HIGH; // high is solenoid off
int isInSettings = 0; // display mode
char* currentPumpStatus = 0;

LiquidCrystal_PCF8574 lcd(0x27);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial){delay(100);}
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

  attachInterrupt(digitalPinToInterrupt(WATER_LOW), onWaterLow, CHANGE);
  attachInterrupt(digitalPinToInterrupt(WATER_HIGH), onWaterFullChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SOL_INLET), onInletTimer, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SOL_SPRINKLERS), onSprinklerTimer, CHANGE);

  // Start by turning everything off
  digitalWrite(INLET_VALVE,LOW);
  digitalWrite(IGNITION,LOW);
  digitalWrite(SPRINKLERS,LOW);
  digitalWrite(STARTER,LOW);

  initLCD();
  lcd.clear();
  lcdWrite(0, "Inlet CLOSED");
  lcdWrite(1, "Pump OFF");
  lcdWrite(2, "Sprnklers CLOSED");
  lcdWrite(3, "W. Level LOW");
  // testLCD();
}

void loop() {
  checkWaterLevel();
  delay(100);


  // if(currentWaterLevel ==1){
  //   Serial.println("Medium");
  // }
  // Serial.print("Current water level ");
  // Serial.println(currentWaterLevel);
  if(currentWaterLevel == 0 && isInletValve(LOW)){
    inletValve(HIGH, 0);
    lastInletTriggerSource = 1;
  } else if(currentWaterLevel == 2 && isInletValve(HIGH) && lastInletTriggerSource==1) {
    inletValve(LOW, 0);
    lastInletTriggerSource = 0;
  }
  if(currentWaterLevel < 2 && inletTimerStatus == LOW && isInletValve(LOW)){
    inletValve(HIGH, 1);
    lastInletTriggerSource = 2; // set trigger source to timer
  } else if(inletTimerStatus == HIGH && isInletValve(HIGH) && lastInletTriggerSource==2) {
    inletValve(LOW, 1);
    lastInletTriggerSource = 0;
  }
  if(sprinklerTimerStatus == LOW && isSprinklers(LOW)){
    sprinklers(HIGH, 1);
  } else if(sprinklerTimerStatus == HIGH && isSprinklers(HIGH)){
    sprinklers(LOW, 1);
  }
  
}
