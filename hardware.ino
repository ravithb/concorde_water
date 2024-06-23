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

bool isPumpStarted(){
  return (digitalRead(PUMP_SENSE)==HIGH);
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
  debugLog("Setting throttle to "+value);
  pwm.setPWM(SERVO_THROTTLE,0,angleToPulse(value));
}

void sprinklers(int status, int source){
  hwBusy = true;
  if(preferences.getUInt(SPRINKLERS_ENABLED, 1)==0){
    sprinklerLine = "Sprinklers DISABLED";
    status = LOW;
    return;
  }

  if(status==HIGH){
    digitalWrite(SPRINKLERS, status);
    sprinklerLine = "Sprinklers Opening..";
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

    bool pumpStatus = startPump(source!=2);
    if(pumpStatus == false){
      status = LOW;
      debugLog("Turning sprinkler valve off");
      digitalWrite(SPRINKLERS, status);      
      sprinklerLine = "Sprinklers Closing..";
      updateScreen = true;
      delay(MECH_VALVE_DELAY);
      sprinklerLine = "Sprnklers CLOSED";
      updateScreen = true;
    }
  }else{
    status = LOW;
    int pumpStatus = stopPump();
    if(pumpStatus==1){
      debugLog("Turning sprinkler valve off");
      digitalWrite(SPRINKLERS, status);
      sprinklerLine = "Sprinklers Closing..";
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
      updateScreen = true;
    }else{
      // turn sprinkler off as the pump failed to stop
      digitalWrite(SPRINKLERS, LOW);
      debugLog("Turning sprinkler valve off due to pump stop failure");
    }
  }
  hwBusy = false;
}

bool areSprinklersOn(){
  return HIGH == digitalRead(SPRINKLERS);
}

bool startPump(bool retry){
  debugLog("Starting pump with "+String(retry?" retries":"no retries"));

  if(preferences.getUInt(PUMP_ENABLED, 1)==0){
    pumpLine = "Pump DISABLED";
    decompression(LOW);
    starterMotor(LOW);
    ignition(LOW);
    updateScreen = true;
    return false;
  }

  pumpLine = "Ignition ON";
  ignition(HIGH);
  updateScreen = true;

  int retries = retry?preferences.getUInt(STARTER_RETRY, 3):1;
  for(int i=0;i<retries;i++){
    debugLog("Pump start attempt "+String(i+1)+" of "+String(retries));
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
    delay(preferences.getUInt(PUMP_SENSE_DELAY, 3)*1000);
    if(isPumpStarted()){
      throttle(preferences.getUInt(THROTTLE_RUN,120));
      pumpLine = "Pump ON";
      updateScreen = true;
      break;
    }else{
      throttle(preferences.getUInt(THROTTLE_STOP,0));
      delay(3000);
      delay(preferences.getUInt(PUMP_RETRY_DELAY,5)*1000);
      pumpLine = "Pump RETRY";
      updateScreen = true;
    }
  }
  if(isPumpStarted()==false){
    pumpLine = "Pump FAILURE";
    updateScreen = true;
    return false;
  }

  return true;
}

int stopPump(){
  int line = 1;
  pumpLine = "Stopping Pump";
  decompression(LOW);
  starterMotor(LOW);
  throttle(preferences.getUInt(THROTTLE_STOP,0));
  delay(3000);
  ignition(LOW);
  updateScreen = true;
  delay(3000);
  int returnStatus = 0;
  if(isPumpStarted()){
    pumpLine = "Pump STOP FAIL";
    returnStatus = 0;
  }else{
    pumpLine = "Pump OFF";
    returnStatus = 1;
  }
  updateScreen = true;
  return returnStatus;
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
    String btLvl = String("LEVEL ")+lvl;
    if(btLvl.equals(btLastLevel)==false || (millis()-lastBtUpdate)>5000){
      writeBTStatus(btLvl);
      btLastLevel = btLvl;
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
  if(isBTConnected()){
    String btSpr = String("SPRINKLERS ")+String((areSprinklersOn()?"ON":"OFF"));
    if(btSpr.equals(btLastSprinklers)==false || (millis()-lastBtUpdate)>5000){
      writeBTStatus(btSpr);
      btLastSprinklers = btSpr;
    }
  }
}

void checkPump() {
  if(preferences.getUInt(PUMP_ENABLED, 1)==false){
    pumpLine = "Pump DISABLED";
    return;
  }
  pumpLine = isPumpStarted()?"Pump ON":"Pump OFF";
  if(lastPump.equals(pumpLine)==false){
    updateScreen = true;
    lastPump = pumpLine;
  }
  if(isBTConnected()){
    String btPmp = String("PUMP ")+String((isPumpStarted())?"ON":"OFF");
    if(btPmp.equals(btLastPump)==false || (millis()-lastBtUpdate)>5000){
      writeBTStatus(btPmp);
      btLastPump = btPmp;
    }
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
    if(btInl.equals(btLastInlet)==false || (millis()-lastBtUpdate)>5000){
      writeBTStatus(btInl);
      btLastInlet = btInl;
    }
  }
}
