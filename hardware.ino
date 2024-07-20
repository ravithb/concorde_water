String lastLvl = "";
String lastSprinklers = "";
String lastPump = "";
String lastInlet = "";
String inletLine = "Inlet --";
String pumpLine = "Pump --";
String sprinklerLine = "Sprinklers --";
String waterLevelLine = "W. Level  --";
String btLastSprinklers = "";
String btLastLevel = "";
String btLastPump = "";
String btLastInlet = "";


bool isPumpRunning(){
  return (digitalRead(PUMP_SENSE)==LOW);
}

void displayStatus(){
  if(isInMenu==false && updateScreen){
    lcdWrite(0, inletLine);
    lcdWrite(1, pumpLine);
    lcdWrite(2, sprinklerLine);
    lcdWrite(3, waterLevelLine);
    updateScreen = false;
  }
}

void displayBusy() {
  if(hwBusy && updateScreen){
    lcdWrite(0, "BUSY");
    lcdWrite(1, "Please wait..");
    lcdWrite(2, " ");
    lcdWrite(3, " ");
    updateScreen = false;
  }
}

void inletValve(int status, int source) {  
  if(preferences.getUInt(INLET_VALVE_ENABLED, 1)==0){
    inletLine = "Inlet DISABLED";
    status = LOW;
    return;
  }
  Serial.print("Status ");
  Serial.println(status);
  Serial.println(!status);
  // Power the solenoid valve for 500 ms
  digitalWrite(IN1, status);
  digitalWrite(IN2, !status);
  digitalWrite(ENA, HIGH);
  delay(SOLND_PWR_TIME);
  digitalWrite(ENA, LOW);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  
  digitalWrite(INLET_VALVE, status);
  if(status==HIGH){  
    
    inletLine = "Inlet OPEN";
    if(source==1){
      inletLine = "Inlet OPEN T";
      debugLog("Turning inlet valve on by timer");
    }else if(source==2){
      inletLine = "Inlet OPEN M";
      debugLog("Turning inlet valve on manually");
    }else {
      debugLog("Turning inlet valve on");
    }
  }else{
    status = LOW;    
    inletLine = "Inlet CLOSED";
    if(source==1){
      inletLine = "Inlet CLOSED T";
      debugLog("Turning inlet valve off by timer");
    }else if(source==2){
      inletLine = "Inlet CLOSED M";
      debugLog("Turning inlet valve off manually");
    }else{
      debugLog("Turning inlet valve off");
    }
  }

}


bool isInletValveOpen(){
  return HIGH == digitalRead(INLET_VALVE);
}

void ignition(int status){
  if(status==HIGH){
    debugLog("Turning ignition on");
  }else{
    status = LOW;
    debugLog("Turning ignition off");
  }
  digitalWrite(IGNITION, status);
}

void starterMotor(int status) {
  if(status==HIGH){
    debugLog("Turning starter motor on");
  }else{
    status = LOW;
    debugLog("Turning starter motor off");
  }
  digitalWrite(STARTER, status);
}

void decompression(int status){
  if(status==HIGH){
    debugLog("Decompression Lever On");
    pwm.setPWM(SERVO_DECOMP, 0, angleToPulse(preferences.getUInt(DECOMP_LEVER_ON, 120)));
  }else{
    debugLog("Decompression Lever Off");
    pwm.setPWM(SERVO_DECOMP, 0, angleToPulse(preferences.getUInt(DECOMP_LEVER_OFF, 0)));
  }
}

void throttle(int value){
  debugLog("Setting throttle to "+String(value));
  pwm.setPWM(SERVO_THROTTLE,0,angleToPulse(value));
}

void sprinklers(int status, int source){
  hwBusy = true;
  if(preferences.getUInt(SPRINKLERS_ENABLED, 1)==0){
    sprinklerLine = "Sprinklers DISABLED";
    updateSprinklerStatusBT("DISABLED");
    status = LOW;
    return;
  }

  if(status==HIGH){
    if(pumpFailureStatus > 0){
      return;
    }
    digitalWrite(SPRINKLERS, status);
    sprinklerLine = "Sprinklers Opening..";
    updateSprinklerStatusBT("OPENING");
    updateScreen = true;
    delay(MECH_VALVE_DELAY); // wait for mechanical valve opener to slowly open the main valve fully before starting the pump

    debugLog("Turning sprinkler valve on");
    sprinklerLine = "Sprinklers OPEN";
    if(source==1){
      sprinklerLine = "Sprinklers OPEN T";
      debugLog("Turning sprinkler valve on by timer");
    }else if(source==2){
      sprinklerLine = "Sprinklers OPEN M";
      debugLog("Turning sprinkler valve on manually");
    }
    updateSprinklerStatusBT("OPEN");
    updatePumpStatusBT("STARTING");
    bool pumpStatus = startPump(source!=2);
    if(pumpStatus == false){
      status = LOW;
      debugLog("Turning sprinkler valve off");
      updateSprinklerStatusBT("CLOSING");
      digitalWrite(SPRINKLERS, status);      
      sprinklerLine = "Sprinklers Closing..";
      updateScreen = true;
      delay(MECH_VALVE_DELAY);
      sprinklerLine = "Sprnklers CLOSED";
      updateSprinklerStatusBT("CLOSED");
      updateScreen = true;
      pumpFailureStatus = 1;
    }else{
      throttle(preferences.getUInt(THROTTLE_RUN,120));
    }
    
  }else{
    status = LOW;
    int pumpStatus = stopPump();
    if(pumpStatus==1){
      debugLog("Turning sprinkler valve off");
      digitalWrite(SPRINKLERS, status);
      sprinklerLine = "Sprinklers Closing..";
      updateSprinklerStatusBT("CLOSING");
      updateScreen = true;
      delay(MECH_VALVE_DELAY);
      
      sprinklerLine = "Sprinklers CLOSED";
      if(source==1){
        sprinklerLine = "Sprinklers CLOSED T";
        debugLog("Turning sprinkler valve off by timer");
      }else if(source==2){
        sprinklerLine = "Sprinklers OPEN M";
        debugLog("Turning sprinkler valve off manually");
      }
      updateSprinklerStatusBT("CLOSED");
      updateScreen = true;
    }
  }
  hwBusy = false;
}

bool areSprinklersOn(){
  return HIGH == digitalRead(SPRINKLERS);
}

bool startPump(bool retry){
  if(isPumpRunning()){
    debugLog("Pump already started");
    return true;
  }
  debugLog("Starting pump with "+String(retry?" retries":"no retries"));

  if(preferences.getUInt(PUMP_ENABLED, 1)==0){
    pumpLine = "Pump DISABLED";
    decompression(LOW);
    starterMotor(LOW);
    ignition(LOW);
    updateScreen = true;
    updatePumpStatusBT("DISABLED");
    return false;
  }

  pumpLine = "Ignition ON";
  ignition(HIGH);
  updateScreen = true;

  int retries = retry?preferences.getUInt(STARTER_RETRY, 3):1;
  for(int i=0;i<retries;i++){
    debugLog("Pump start attempt "+String(i+1)+" of "+String(retries));
    updatePumpStatusBT("START "+String(i+1));
    pumpLine = "Decomp On";
    decompression(HIGH);
    updateScreen = true;

    throttle(preferences.getUInt(THROTTLE_START,90));

    delay(preferences.getUInt(STARTER_DELAY, 2)*1000);
    pumpLine = "Starting Pump";
    starterMotor(HIGH);
    updateScreen = true;

    delay(preferences.getUInt(DECOMP_LEVER_DELAY,1)*1000);
    pumpLine = "Decomp Off";
    decompression(LOW);
    updateScreen = true;

    delay(preferences.getUInt(CRANK_TIME, 3)*1000);
    starterMotor(LOW);
    updatePumpStatusBT("SENSING");
    delay(preferences.getUInt(PUMP_SENSE_DELAY, 3)*1000);
    if(isPumpRunning()){
      debugLog("Pump runing signal detected");
      throttle(preferences.getUInt(THROTTLE_RUN,120));
      pumpLine = "Pump ON";
      updateScreen = true;
      updatePumpStatusBT("ON");
      break;
    }else{
      debugLog("Pump running signal not detected");
      throttle(preferences.getUInt(THROTTLE_STOP,0));
      delay(3000);
      delay(preferences.getUInt(PUMP_RETRY_DELAY,5)*1000);
      pumpLine = "Pump RETRY";
      updatePumpStatusBT("RETRY");
      updateScreen = true;
    }
  }
  if(isPumpRunning()==false){
    pumpLine = "Pump Start Failed";
    updateScreen = true;
    updatePumpStatusBT("START FAILED");
    return false;
  }

  return true;
}

int stopPump(){
  if(isPumpRunning()==false){
    return 2;
  }
  int line = 1;
  pumpLine = "Stopping Pump";
  updatePumpStatusBT("STOPPING");
  decompression(LOW);
  starterMotor(LOW);
  throttle(preferences.getUInt(THROTTLE_STOP,0));
  delay(3000);
  ignition(LOW);
  updateScreen = true;
  delay(5000);
  int returnStatus = 0;
  if(isPumpRunning()){
    pumpLine = "Pump STOP FAIL";
    updatePumpStatusBT("STOP FAILED");
    returnStatus = 0;
  }else{
    pumpLine = "Pump OFF";
    updatePumpStatusBT("OFF");
    returnStatus = 1;    
  }
  updateScreen = true;
  return returnStatus;
}

void handlePumpFailure(){
  debugLog("Closing throttle due to failure");
  decompression(LOW);
  starterMotor(LOW);
  throttle(preferences.getUInt(THROTTLE_STOP,0));
  ignition(LOW);

  debugLog("Closing sprinklers.");
  digitalWrite(SPRINKLERS, LOW);

}

// check periodically and turn the pump off if tank is full
void checkWaterLevel(){
  if(digitalRead(WATER_HIGH)==LOW){
    onWaterFullChange();  
  }
  String lvl = "";
  if(currentWaterLevel == 2){
    lvl = "FULL";
  }else if (currentWaterLevel == 1) {
    lvl = "MED";
  }else {
    lvl = "LOW";
  }
  waterLevelLine = "W. Level " + lvl;
  if(lvl.equals(lastLvl)==false){    
    lastLvl = lvl;
    debugLog("last lvl "+lastLvl);
    updateScreen = true;
  }  
  if(isBTConnected()){
    // Serial.println("Set BT Level "+lvl+ " " +btLastLevel+ " "+ String((millis()-lastBtUpdate)>5000));
    String btLvl = String("LEVEL ")+lvl;
    if(btLvl.equals(btLastLevel)==false || (millis()-lastBtUpdateLvl)>5000){
      writeBTStatus(btLvl);
      lastBtUpdateLvl = millis();
      btLastLevel = btLvl;
    }
  }
}

void updateSprinklerStatusBT(String status){
  if(isBTConnected()){
    String btSpr = String("SPRINKLERS ")+status;
    if(btSpr.equals(btLastSprinklers)==false || (millis()-lastBtUpdateSpr)>5000){
      writeBTStatus(btSpr);
      lastBtUpdateSpr = millis();
      btLastSprinklers = btSpr;
    }
  }
}

void checkSprinklers() {
  if(preferences.getUInt(SPRINKLERS_ENABLED, 1)==0){
    sprinklerLine = "Sprinklers DISABLED";
    return;
  }
  sprinklerLine = (areSprinklersOn()?"Sprinklers OPEN":"Sprnklers CLOSED");
  if(lastSprinklers.equals(sprinklerLine)==false){
    updateScreen = true;
    lastSprinklers = sprinklerLine;
  }
  updateSprinklerStatusBT(String((areSprinklersOn()?"OPEN":"CLOSED")));
}

void updatePumpStatusBT(String status ){
  if(isBTConnected()){
    String btPmp = String("PUMP ")+status;
    if(btPmp.equals(btLastPump)==false || (millis()-lastBtUpdatePmp)>5000){
      writeBTStatus(btPmp);
      lastBtUpdatePmp = millis();
      btLastPump = btPmp;
    }
  }
}

void checkPump() {
  if(preferences.getUInt(PUMP_ENABLED, 1)==false){
    pumpLine = "Pump DISABLED";
    return;
  }
  if(pumpFailureStatus > 0) {
    switch (pumpFailureStatus) {
      case 1:
        pumpLine = "Pump Start Failed";
        updatePumpStatusBT(String("START FAILED"));
        break;
      case 2:
        pumpLine = "Pump Run Failed";
        updatePumpStatusBT(String("RUN FAILED"));
        break;
    }
    updateScreen = true;
    
  }else{
    pumpLine = isPumpRunning()?"Pump ON":"Pump OFF";
    if(lastPump.equals(pumpLine)==false){
      updateScreen = true;
      lastPump = pumpLine;
    }
    updatePumpStatusBT(String((isPumpRunning())?"ON":"OFF"));
  }
}

void checkInletValve(){
  if(preferences.getUInt(INLET_VALVE_ENABLED, 1)==0){
    inletLine = "Inlet DISABLED";
    return;
  }
  inletLine = isInletValveOpen()?"Inlet OPEN":"Inlet CLOSED";
  if(lastInlet.equals(inletLine)==false){
    updateScreen = true;
    lastInlet = inletLine;
  }
  if(isBTConnected()){
    String btInl = String("INLET ")+String((isInletValveOpen())?"OPEN":"CLOSED");
    if(btInl.equals(btLastInlet)==false || (millis()-lastBtUpdateInl)>5000){
      writeBTStatus(btInl);
      lastBtUpdateInl = millis();
      btLastInlet = btInl;
    }
  }
}
