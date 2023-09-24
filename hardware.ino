String lastLvl = "";
String lastSprinklers = "";
String lastPump = "";
String lastInlet = "";
String inletLine = "Inlet --";
String pumpLine = "Pump --";
String sprinklerLine = "Sprinklers --";
String waterLevelLine = "W. Level  --";

bool isPumpStarted(){
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

void sprinklers(int status, int isFromTimer){
  if(preferences.getUInt("sprinklersEnabled", 1)==0){
    sprinklerLine = "Sprinklers DISABLED";
    status = LOW;
    return;
  }

  digitalWrite(SPRINKLERS, status);

  if(status==HIGH){
    startPump();
    Serial.println("Turning sprinkler valve on");
    sprinklerLine = "Sprinklers OPEN";
    if(isFromTimer==1){
      sprinklerLine = "Sprinklers OPEN T";
    }
  }else{
    status = LOW;
    stopPump();
    Serial.println("Turning sprinkler valve off");
    sprinklerLine = "Sprnklers CLOSED";
    if(isFromTimer==1){
      sprinklerLine = "Sprnklers CLOSED T";
    }
  }
  
}

bool areSprinklersOn(){
  return HIGH == digitalRead(SPRINKLERS);
}

void startPump(){
  if(preferences.getUInt(PUMP_ENABLED, 1)==0){
    pumpLine = "Pump DISABLED";
    starterMotor(LOW);
    ignition(LOW);
    displayStatus();
    return;
  }

  pumpLine = "Ignition ON";
  ignition(HIGH);
  displayStatus();

  int retries = preferences.getUInt(STARTER_RETRY, 3);
  for(int i=0;i<retries;i++){
    delay(preferences.getUInt(STARTER_DELAY, 2)*1000);
    pumpLine = "Starting Pump";
    starterMotor(HIGH);
    displayStatus();
    delay(preferences.getUInt(CRANK_TIME, 3)*1000);
    starterMotor(LOW);
    if(isPumpStarted()){
      pumpLine = "Pump ON";
      displayStatus();
      break;
    }else{
      delay(preferences.getUInt(PUMP_RETRY_DELAY,5)*1000);
      pumpLine = "Pump RETRY";
      displayStatus();
    }
  }
  if(isPumpStarted()==false){
    pumpLine = "Pump FAILURE";
    displayStatus();
  }
}

void stopPump(){
  int line = 1;
  pumpLine = "Stopping Pump";
  starterMotor(LOW);
  ignition(LOW);
  displayStatus();
  delay(3000);
  if(isPumpStarted()){
    pumpLine = "Pump STOP FAIL";
  }else{
    pumpLine = "Pump OFF";
  }
  displayStatus();
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
