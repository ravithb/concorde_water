String timeStr = "";
long oldPosition  = -999;
long initPosition = -999;
unsigned long menuTriggeredTime = 0;
const int numOfScreens = 17;
const String TYPE_ON_OFF_EXIT = "ON_OFF_EXIT";
const String TYPE_EN_DIS = "EN_DIS";
const String TYPE_INT = "INT";
const String TYPE_NONE = "NONE";
const String ON_OFF_EXIT[3] = {"EXIT","OFF","ON"};
const String EN_DIS[2] = {"DISABLED","ENABLED"};

int currentScreen = -1;
int lastScreen = -1;
String screens[numOfScreens][3] = {
  {"Manual Watering","", TYPE_ON_OFF_EXIT},
  {"Manual Filling","", TYPE_ON_OFF_EXIT},
  {"Inlet","",TYPE_EN_DIS}, 
  {"Irrigation","", TYPE_EN_DIS},
  {"Pump","", TYPE_EN_DIS},
  {"Starter Retry", "Times", TYPE_INT}, 
  {"Starter Delay", "Seconds", TYPE_INT}, 
  {"Crank Time", "Seconds", TYPE_INT}, 
  {"Pump Sense Delay", "Seconds", TYPE_INT},
  {"Retry Delay","Seconds", TYPE_INT},
  {"Dcomp Delay","Seconds", TYPE_INT},
  {"Dcomp Off Pos", "Degrees", TYPE_INT},
  {"Dcomp On Pos", "Degrees", TYPE_INT}, 
  {"Thr Start Pos", "Degrees", TYPE_INT},  
  {"Thr Run Pos", "Degrees", TYPE_INT},
  {"Thr Stop Pos", "Degrees", TYPE_INT},
  {"Test Pump", "", TYPE_ON_OFF_EXIT }

};
int minMax[numOfScreens][2] = {
  {0,2},//Manual Watering
  {0,2},//Manual Filling
  {0,1},//Inlet
  {0,1},//Irrigation
  {0,1},//Pump
  {3,10},//Starter Retry
  {2,10},//Starter Delay
  {1,5},//Crank Time
  {1,5},//Pump Sense Dealay
  {2,10},//Retry Delay
  {1,3},//Decompression Leaver Delay
  {0,120},//Decomp Lever Off Position
  {0,120},//Decomp Lever On Position
  {0,120},//Thtottle Start Position
  {0,120},//Thtottle Run Position
  {0,120},//Thtottle Off Position
  {0,2}, // Test Pump
};
int parameters[numOfScreens];

void loadParameterValues(){
    //Inlet
    parameters[2] = preferences.getUInt(INLET_VALVE_ENABLED, true);
    //Irrigation
    parameters[3] = preferences.getUInt(SPRINKLERS_ENABLED, true);
    //Pump
    parameters[4] = preferences.getUInt(PUMP_ENABLED, true);
    //Starter Retry
    parameters[5] = preferences.getUInt(STARTER_RETRY, 3);
    //Starteer Delay
    parameters[6] = preferences.getUInt(STARTER_DELAY, 2);
    //Crank Time
    parameters[7] = preferences.getUInt(CRANK_TIME, 3);
    //Pumb Sense Delay
    parameters[8] = preferences.getUInt(PUMP_SENSE_DELAY, 1);    
    //Retry Delay
    parameters[9] = preferences.getUInt(PUMP_RETRY_DELAY,5);
    //Decompression Lever Delay
    parameters[10] = preferences.getUInt(DECOMP_LEVER_DELAY,1);
    //Decompresion Lever Off Position
    parameters[11] = preferences.getUInt(DECOMP_LEVER_OFF,0);
    //Decompresion Lever On Position
    parameters[12] = preferences.getUInt(DECOMP_LEVER_ON,120);
    //Thtottle Start Position
    parameters[13] = preferences.getUInt(THROTTLE_START,90);
    //Thtottle Run Position
    parameters[14] = preferences.getUInt(THROTTLE_RUN,120);
    //Thtottle Stop Position
    parameters[15] = preferences.getUInt(THROTTLE_STOP,0);
}

int getCurrentVal(int screenIdx){
  switch(screenIdx){
    case 2: //Inlet
      return preferences.getUInt(INLET_VALVE_ENABLED, true);
    case 3: //Irrigation
      return preferences.getUInt(SPRINKLERS_ENABLED, true);
    case 4: //Pump
      return preferences.getUInt(PUMP_ENABLED, true);
    case 5: //Starter Retry
      return preferences.getUInt(STARTER_RETRY, 3);
    case 6: //Starteer Delay
      return preferences.getUInt(STARTER_DELAY, 2);
    case 7: //Crank Time
      return preferences.getUInt(CRANK_TIME, 3);
    case 8: //Pump Sense Delay
      return preferences.getUInt(PUMP_SENSE_DELAY, 1);
    case 9: //Retry Delay
      return preferences.getUInt(PUMP_RETRY_DELAY,5);
    case 10: //Decompression Lever Delay
      return preferences.getUInt(DECOMP_LEVER_DELAY,1);
    case 11: //Decompresion Lever Off Position
      return preferences.getUInt(DECOMP_LEVER_OFF,0);
    case 12: //Decompresion Lever On Position
      return preferences.getUInt(DECOMP_LEVER_ON,120);
    case 13: //Thtottle Start Position
      return preferences.getUInt(THROTTLE_START,90);
    case 14: //Thtottle Run Position
      return preferences.getUInt(THROTTLE_RUN,120);
    case 15: //Thtottle Stop Position
      return preferences.getUInt(THROTTLE_STOP,0);
  }
}

String getParameterVal(String type, unsigned int val) {
  if(TYPE_ON_OFF_EXIT.equals(type)){
    return ON_OFF_EXIT[val%3];
  }else if(TYPE_EN_DIS.equals(type)){
    return EN_DIS[val%2];
  }else{
    return String(val);
  }
}

void setActiveOption(int screenIdx){
  
  switch(screenIdx){
    case 0:
      // Serial.print("Manual Watering ");
      // Serial.println(parameters[screenIdx]);
      selectedActiveOptParam = parameters[screenIdx] % 3;
    break;
    case 1:
      // Serial.print("Manual Filling ");
      // Serial.println(parameters[screenIdx]);
      selectedActiveOptParam = parameters[screenIdx] % 3;
    break;
    case 16:
      // Serial.print("Pump Test ");
      // Serial.println(parameters[screenIdx]);
      selectedActiveOptParam = parameters[screenIdx] % 3;
    break;
  }
  selectedActiveOption = screenIdx;
  debugLog("Set active option "+String(selectedActiveOption)+" with param "+String(selectedActiveOptParam));
}

void runActiveOption(){
  
  bool exitMenuFlag = false;
  switch(selectedActiveOption){
    case 0: // manual watering
      debugLog("Run manual watering");
      manualWatering = selectedActiveOptParam;
      runActiveOptionFlag = true;
      exitMenuFlag = true;
      break;
    case 1: // manual filling
      debugLog("Run manual filling");
      manualFilling = selectedActiveOptParam;
      runActiveOptionFlag = true;
      exitMenuFlag = true;
      break;
    case 16: // pump test
      debugLog("Run pump test");
      pumpTest = selectedActiveOptParam;
      runActiveOptionFlag = true;
      exitMenuFlag = true;
      break;
  }
  if(exitMenuFlag){
    debugLog("Exit menu on active option selection");
    exitMenu();
  }
}

void resetActiveOptionMenu(){
  parameters[0] = minMax[0][0]; // Manual watering
  parameters[1] = minMax[1][0]; // Manual filling
  parameters[16] = minMax[16][0]; // Pump test
}

void resetSelectedActiveOption(){
  if(selectedActiveOption >=0 && runActiveOptionFlag == true){
    selectedActiveOption = -1;
    selectedActiveOptParam = -1;
    runActiveOptionFlag = false;
    resetActiveOptionMenu();
  }
}

bool isActiveOptionSet(){
  return selectedActiveOption >=0 && selectedActiveOptParam >=0;
}

void saveParameter(int screenIdx){
    
  switch(screenIdx){
    case 2: //Inlet
      Serial.print("Saving Inlet ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(INLET_VALVE_ENABLED, parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(INLET_VALVE_ENABLED,99));
      break;
    case 3: //Irrigation
      Serial.print("Saving Sprinklers ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(SPRINKLERS_ENABLED, parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(SPRINKLERS_ENABLED,99));
      break;
    case 4: //Pump
    Serial.print("Saving Pump ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(PUMP_ENABLED, parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(PUMP_ENABLED,99));
      break;
    case 5: //Starter Retry
      Serial.print("Saving Starter Retry ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(STARTER_RETRY, parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(STARTER_RETRY,99));
      break;
    case 6: //Starter Delay
      Serial.print("Saving Starter Delay ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(STARTER_DELAY, parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(STARTER_DELAY,99));
      break;
    case 7: //Crank Time
      Serial.print("Saving Crank Time ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(CRANK_TIME, parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(CRANK_TIME,99));
      break;
    case 8: //Crank Time
      Serial.print("Saving Pump Sense Delay ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(PUMP_SENSE_DELAY, parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(PUMP_SENSE_DELAY,99));
      break;
    case 9: //Retry Delay
      Serial.print("Saving Pump Retry Delay ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(PUMP_RETRY_DELAY,parameters[screenIdx]);
      Serial.print("Read back ");Serial.println(menuInterrupt);
      Serial.println(preferences.getUInt(PUMP_RETRY_DELAY,99));
      break;
    case 10: //Decompression Lever Delay
      Serial.print("Saving Decompression Lever Delay ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(DECOMP_LEVER_DELAY,parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(DECOMP_LEVER_DELAY,1));
      break;
    case 11: //Decompresion Lever Off
      Serial.print("Saving Decompression Lever Off Position ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(DECOMP_LEVER_OFF,parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(DECOMP_LEVER_OFF,0));
      break;
    case 12: //Decompresion Lever Off
      Serial.print("Saving Decompression Lever On Position ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(DECOMP_LEVER_ON,parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(DECOMP_LEVER_ON,120));
      break;
    case 13: //Throttle Start Position
      Serial.print("Saving Throttle Start Position Degrees ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(THROTTLE_START,parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(THROTTLE_START,90));
      break;
    case 14: //Throttle Run Position
      Serial.print("Saving Throttle Run Position Degrees ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(THROTTLE_RUN,parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(THROTTLE_RUN,120));
      break;
    case 15://Throttle Stop Position
    case -2:
      Serial.print("Saving Throttle Stop Position Degrees ");
      Serial.println(parameters[screenIdx]);
      preferences.putUInt(THROTTLE_STOP,parameters[screenIdx]);
      Serial.print("Read back ");
      Serial.println(preferences.getUInt(THROTTLE_STOP,0));
      break;
  }
}

void exitMenu(){
  setKeepBacklightOn(false);
  menuTriggeredTime = 0;
  currentScreen = -1;
  menuInterrupt = false;
  isInMenu = false;
  updateScreen = true;
  displayStatus();
}

void previewServoPosition(int idx){
  switch(idx){
    case 13:
    case 14:
    case 15:
      throttle(parameters[idx]);
      break;
    case 11:
    case 12:
      pwm.setPWM(SERVO_DECOMP, 0, angleToPulse(parameters[idx]));
      break;
  }
}

void menu(){
  // Serial.print("trigger ");
  // Serial.println(menuInterrupt);
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
      previewServoPosition(currentScreen);
      if(currentScreen > 1 && currentScreen < 16){
        saveParameter(currentScreen);
      }else{
        setActiveOption(currentScreen);
      }
      menuInterrupt = false;
      //reset menu trigger time on parameter change
      menuTriggeredTime = millis();
      updateScreen = true;
    }
   
    oldPosition = newPosition;


  }

  if(menuInterrupt==true && selectedActiveOption>-1){ 
    // active option triggered
    runActiveOption();
    
  }



  if(menuTriggeredTime != 0){
    if(currentScreen != lastScreen) {
      saveParameter(lastScreen);
      lastScreen = currentScreen;
    }

    if(currentScreen != -1) {
      if(hwBusy){
        updateScreen = true;
        displayBusy();
        delay(2000);
        exitMenu();
        return;
      }else{
        displayMenu();
      }
      if(menuTriggeredTime + 10000 < millis()) {
        debugLog("Exit menu on timeout");
        exitMenu();

        enc.resetPosition(initPosition, false);
        oldPosition = initPosition;
        newPosition = initPosition;
        initPosition = -999;
      }
    }else{
      debugLog("Exit menu on end");
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
    if(currentScreen == 0 || currentScreen == 1 || currentScreen == 16){
      String subMenuSelection = getParameterVal(screens[currentScreen][2], parameters[currentScreen]);
      if(subMenuSelection.equals("EXIT")){
        currentScreen++;
      }
    } else {
      currentScreen++;      
    }
    if(currentScreen >= numOfScreens) {
      currentScreen = -1;    
      // this exists the menu  
      menuInterrupt = false;    
    }
        
    updateScreen = true;
  }
}

void displayMenu() {
  isInMenu = true;
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
