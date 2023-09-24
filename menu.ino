String timeStr = "";
long oldPosition  = -999;
long initPosition = -999;
unsigned long menuTriggeredTime = 0;
const int numOfScreens = 7;
const String TYPE_ON_OFF = "ON_OFF";
const String TYPE_EN_DIS = "EN_DIS";
const String TYPE_INT = "INT";
const String ON_OFF[2] = {"OFF","ON"};
const String EN_DIS[2] = {"DISABLED","ENABLED"};

int currentScreen = -1;
int lastScreen = -1;
String screens[numOfScreens][3] = {
  {"Inlet","",TYPE_EN_DIS}, 
  {"Irrigation","", TYPE_EN_DIS},
  {"Pump","", TYPE_EN_DIS},
  {"Starter Retry", "Times", TYPE_INT}, 
  {"Starter Delay", "Seconds", TYPE_INT}, 
  {"Crank Time", "Seconds", TYPE_INT}, 
  {"Retry Delay","Seconds", TYPE_INT}   
};
int minMax[numOfScreens][2] = {
  {0,1},//Inlet
  {0,1},//Irrigation
  {0,1},//Pump
  {3,10},//Starter Retry
  {2,10},//Starter Delay
  {2,10},//Crank Time
  {2,10},//Retry Delay
};
int parameters[numOfScreens];

void loadParameterValues(){
    //Inlet
    parameters[0] = preferences.getUInt(INLET_VALVE_ENABLED, true);
    //Irrigation
    parameters[1] = preferences.getUInt(SPRINKLERS_ENABLED, true);
    //Pump
    parameters[2] = preferences.getUInt(PUMP_ENABLED, true);
    //Starter Retry
    parameters[3] = preferences.getUInt(STARTER_RETRY, 3);
    //Starteer Delay
    parameters[4] = preferences.getUInt(STARTER_DELAY, 2);
    //Crank Time
    parameters[5] = preferences.getUInt(CRANK_TIME, 3);
    //Retry Delay
    parameters[6] = preferences.getUInt(PUMP_RETRY_DELAY,5);
}

int getCurrentVal(int screenIdx){
  switch(screenIdx){
    case 0: //Inlet
      return preferences.getUInt(INLET_VALVE_ENABLED, true);
    case 1: //Irrigation
      return preferences.getUInt(SPRINKLERS_ENABLED, true);
    case 2: //Pump
      return preferences.getUInt(PUMP_ENABLED, true);
    case 3: //Starter Retry
      return preferences.getUInt(STARTER_RETRY, 3);
    case 4: //Starteer Delay
      return preferences.getUInt(STARTER_DELAY, 2);
    case 5: //Crank Time
      return preferences.getUInt(CRANK_TIME, 3);
    case 6: //Retry Delay
      return preferences.getUInt(PUMP_RETRY_DELAY,5);
  }
}

String getParameterVal(String type, unsigned int val) {
  if(TYPE_ON_OFF.equals(type)){
    return ON_OFF[val%2];
  }else if(TYPE_EN_DIS.equals(type)){
    return EN_DIS[val%2];
  }else{
    return String(val);
  }
}

void saveParameter(int screenIdx){
  Serial.print("Saving ");
  
  switch(screenIdx){
    case 0: //Inlet
      Serial.print("Inlet ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(INLET_VALVE_ENABLED, parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(INLET_VALVE_ENABLED,99));
      break;
    case 1: //Irrigation
      Serial.print("Sprinklers ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(SPRINKLERS_ENABLED, parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(SPRINKLERS_ENABLED,99));
      break;
    case 2: //Pump
    Serial.print("Pump ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(PUMP_ENABLED, parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(PUMP_ENABLED,99));
      break;
    case 3: //Starter Retry
      Serial.print("Starter Retry ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(STARTER_RETRY, parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(STARTER_RETRY,99));
      break;
    case 4: //Starter Delay
      Serial.print("Starter Delay ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(STARTER_DELAY, parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(STARTER_DELAY,99));
      break;
    case 5: //Crank Time
      Serial.print("Crank Time ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(CRANK_TIME, parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(CRANK_TIME,99));
      break;
    case -2:
    default:
      //Retry Delay
      Serial.print("Pump Retry Delay ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(PUMP_RETRY_DELAY,parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(PUMP_RETRY_DELAY,99));
      break;
  }
}

void exitMenu(){
  setKeepBacklightOn(false);
  menuTriggeredTime = 0;
  currentScreen = -1;
  menuInterrupt = false;
  updateScreen = true;
  displayStatus();
}

void menu(){
  // Serial.print("Current scr ");
  // Serial.println(currentScreen);
  long newPosition = enc.getPosition();
  if (newPosition != oldPosition /* && newPosition % 2 == 0 */) {
    if(menuTriggeredTime != 0 && currentScreen != -1) {
      if(newPosition > oldPosition) {
        parameters[currentScreen]++;
      } else {
        parameters[currentScreen]--;
      }
      // Serial.print("Param ");
      // Serial.println(parameters[currentScreen]);
      if(parameters[currentScreen] < minMax[currentScreen][0]){
        parameters[currentScreen] = minMax[currentScreen][0];
      }
      if(parameters[currentScreen] > minMax[currentScreen][1]){
        parameters[currentScreen] = minMax[currentScreen][1];
      }

      saveParameter(currentScreen);
      
      //reset menu trigger time on parameter change
      menuTriggeredTime = millis();
      updateScreen = true;
    }

    oldPosition = newPosition;
  }

  if(menuTriggeredTime != 0){
    if(currentScreen != lastScreen) {
      // Serial.print("Saving last screen ");
      // Serial.println(lastScreen);
      saveParameter(lastScreen);
      lastScreen = currentScreen;
    }

    if(currentScreen != -1) {
      displayMenu();
      if(menuTriggeredTime + 10000 < millis()) {
        exitMenu();
        // Serial.println("Init pos:");
        // Serial.println(initPosition);
        enc.resetPosition(initPosition, false);
        oldPosition = initPosition;
        newPosition = initPosition;
        initPosition = -999;
      }
    }else{
      exitMenu();
    }   

  }

  delay(10);
}

// void updateTime(String tStr) {
//   if(timeStr != tStr) {
//     lcd.setCursor(0,0);
//     lcd.print(tStr);
//     timeStr = tStr;
//   }
// }

#define countof(a) (sizeof(a) / sizeof(a[0]))

void triggerMenu()
{
  if(menuTriggeredTime + 50 < millis()){
    if(menuTriggeredTime == 0) {
      initPosition = oldPosition;
    }
    menuTriggeredTime = millis();
    currentScreen++;
    if(currentScreen >= numOfScreens) {
      currentScreen = -1;
      menuInterrupt = false;
    }
    updateScreen = true;
  }
}

void displayMenu() {
  if(updateScreen) {
    setKeepBacklightOn(true);
    lcd.clear();
    lcd.print(" ***  SETTINGS  *** ");
    lcd.setCursor(0,1);
    lcd.print(screens[currentScreen][0]);
    lcd.setCursor(0,2);
    lcd.print(getParameterVal(screens[currentScreen][2], parameters[currentScreen]));
    lcd.print(" ");
    lcd.print(screens[currentScreen][1]);
    updateScreen = false;
  }
}
