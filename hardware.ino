String lastLvl = "";
String lastSprinklers = "";
String lastPump = "";
String lastInlet = "";
String inletLine = "Inlet --";
String pumpLine = "Pump --";
String sprinklerLine = "Sprinklers --";
String waterLevelLine = "W. Level  --";

bool isPumpStarted(){
  // Serial.print("Pump started ? ");
  // Serial.println(digitalRead(PUMP_SENSE)==HIGH);
  return (digitalRead(PUMP_SENSE)==HIGH);
}

void displayStatus(){
  if(updateScreen){
    lcdWrite(0, inletLine);
    lcdWrite(1, pumpLine);
    lcdWrite(2, sprinklerLine);
    lcdWrite(3, waterLevelLine);
    updateScreen = false;
  }
}

void inletValve(int status, int isFromTimer) {  
  if(preferences.getUInt(INLET_VALVE_ENABLED, 1)==0){
    inletLine = "Inlet DISABLED";
    status = LOW;
    return;
  }

  digitalWrite(INLET_VALVE, status);
  if(status==HIGH){    
    Serial.println("Turning inlet valve on");
    inletLine = "Inlet OPEN";
    if(isFromTimer==1){
      inletLine = "Inlet OPEN T";
    }
  }else{
    status = LOW;
    Serial.println("Turning inlet valve off");
    inletLine = "Inlet CLOSED";
    if(isFromTimer==1){
      inletLine = "Inlet CLOSED T";
    }
  }

}


bool isInletValveOpen(){
  return HIGH == digitalRead(INLET_VALVE);
}

void ignition(int status){
  if(status==HIGH){
    Serial.println("Turning ignition on");
  }else{
    status = LOW;
    Serial.println("Turning ignition off");
  }
  digitalWrite(IGNITION, status);
}

void starterMotor(int status) {
  if(status==HIGH){
    Serial.println("Turning starter motor on");
  }else{
    status = LOW;
    Serial.println("Turning starter motor off");
  }
  digitalWrite(STARTER, status);
}

void decompression(int status){
  if(status==HIGH){
    Serial.println("Decompression Lever On");
    pwm.setPWM(SERVO_DECOMP, 0, angleToPulse(preferences.getUInt(DECOMP_LEVER_ON, 120)));
  }else{
    Serial.println("Decompression Lever Off");
    pwm.setPWM(SERVO_DECOMP, 0, angleToPulse(preferences.getUInt(DECOMP_LEVER_OFF, 0)));
  }
}

void throttle(int value){
  Serial.print("Setting throttle to ");
  Serial.println(value);
  pwm.setPWM(SERVO_THROTTLE,0,angleToPulse(value));
}

void sprinklers(int status, int isFromTimer){
  if(preferences.getUInt(SPRINKLERS_ENABLED, 1)==0){
    sprinklerLine = "Sprinklers DISABLED";
    status = LOW;
    return;
  }

  if(status==HIGH){
    digitalWrite(SPRINKLERS, status);
    Serial.println("Turning sprinkler valve on");
    sprinklerLine = "Sprinklers OPEN";
    if(isFromTimer==1){
      sprinklerLine = "Sprinklers OPEN T";
    }
    displayStatus();
    delay(30000); // wait for mechanical valve opener to slowly open the main valve fully before starting the pump
    bool pumpStatus = startPump();
    if(pumpStatus == false){
      status = LOW;
      digitalWrite(SPRINKLERS, status);
      Serial.println("Turning sprinkler valve off");
      sprinklerLine = "Sprnklers CLOSED";
      displayStatus();
    }
  }else{
    status = LOW;
    int pumpStatus = stopPump();
    if(pumpStatus==1){
      digitalWrite(SPRINKLERS, status);
      Serial.println("Turning sprinkler valve off");
      sprinklerLine = "Sprnklers CLOSED";
      if(isFromTimer==1){
        sprinklerLine = "Sprnklers CLOSED T";
      }
    }else{
      // leave sprinkler on as the pump failed to stop
      digitalWrite(SPRINKLERS, HIGH);
    }
  }
  
}

bool areSprinklersOn(){
  return HIGH == digitalRead(SPRINKLERS);
}

bool startPump(){
  if(preferences.getUInt(PUMP_ENABLED, 1)==0){
    pumpLine = "Pump DISABLED";
    decompression(LOW);
    starterMotor(LOW);
    ignition(LOW);
    displayStatus();
    return false;
  }

  pumpLine = "Ignition ON";
  ignition(HIGH);
  displayStatus();

  int retries = preferences.getUInt(STARTER_RETRY, 3);
  for(int i=0;i<retries;i++){
    pumpLine = "Decomp On";
    decompression(HIGH);
    updateScreen = true;
    displayStatus();

    throttle(preferences.getUInt(THROTTLE_START,90));

    delay(preferences.getUInt(STARTER_DELAY, 2)*1000);
    pumpLine = "Starting Pump";
    starterMotor(HIGH);
    updateScreen = true;
    displayStatus();

    delay(preferences.getUInt(DECOMPR_LEVER_DELAY,1)*1000);
    pumpLine = "Decomp Off";
    decompression(LOW);
    updateScreen = true;
    displayStatus();

    delay(preferences.getUInt(CRANK_TIME, 3)*1000);
    starterMotor(LOW);
    delay(preferences.getUInt(PUMP_SENSE_DELAY, 3)*1000);
    if(isPumpStarted()){
      throttle(preferences.getUInt(THROTTLE_RUN,120));
      pumpLine = "Pump ON";
      updateScreen = true;
      displayStatus();
      break;
    }else{
      throttle(preferences.getUInt(THROTTLE_STOP,0));
      delay(3000);
      delay(preferences.getUInt(PUMP_RETRY_DELAY,5)*1000);
      pumpLine = "Pump RETRY";
      updateScreen = true;
      displayStatus();
    }
  }
  if(isPumpStarted()==false){
    pumpLine = "Pump FAILURE";
    updateScreen = true;
    displayStatus();
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
  displayStatus();
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
  displayStatus();
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
    Serial.println("last lvl "+lastLvl);
    updateScreen = true;
    displayStatus();
  }  
  
}

void checkSprinklers() {
  if(preferences.getUInt(SPRINKLERS_ENABLED, 1)==0){
    sprinklerLine = "Sprinklers DISABLED";
    return;
  }
  sprinklerLine = areSprinklersOn()?"Sprinklers OPEN":"Sprnklers CLOSED";
  if(lastSprinklers.equals(sprinklerLine)==false){
    updateScreen = true;
    lastSprinklers = sprinklerLine;
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
}
