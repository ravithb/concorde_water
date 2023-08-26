String lastLvl = "";

void inletValve(int status, int isFromTimer) {
  String msg = "";
  if(status==HIGH){
    
    Serial.println("Turning inlet valve on");
    msg = "Inlet OPEN";
    if(isFromTimer==1){
      msg = "Inlet OPEN T";
    }
  }else{
    status = LOW;
    Serial.println("Turning inlet valve off");
    msg = "Inlet CLOSED";
    if(isFromTimer==1){
      msg = "Inlet CLOSED T";
    }
  }
  lcdWrite(0, msg);
  digitalWrite(INLET_VALVE, status);
}


bool isInletValve(int status){
  return status == digitalRead(INLET_VALVE);
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
  int line = 2;
  if(status==HIGH){
    startPump();
    Serial.println("Turning sprinkler valve on");
    lcdWrite(line, "Sprinklers OPEN");
    if(isFromTimer==1){
      lcdWrite(line, "Sprinklers OPEN T");
    }
  }else{
    status = LOW;
    stopPump();
    Serial.println("Turning sprinkler valve off");
    lcdWrite(line, "Sprnklers CLOSED");
    if(isFromTimer==1){
      lcdWrite(line, "Sprnklers CLOSED T");
    }
  }
  
  digitalWrite(SPRINKLERS, status);
}

bool isSprinklers(int status){
  return status == digitalRead(SPRINKLERS);
}

void startPump(){
  int line = 1;
  lcdWrite(line,"Ignition ON");
  ignition(HIGH);
  delay(2000);
  lcdWrite(line,"Starting Pump");
  starterMotor(HIGH);
  delay(3000);
  starterMotor(LOW);
  lcdWrite(line,"Pump ON");
}

void stopPump(){
  int line = 1;
  lcdWrite(line,"Stopping Pump");
  starterMotor(LOW);
  ignition(LOW);
  lcd.setCursor(0,0);
  lcdWrite(line,"Pump OFF");
  delay(3000);
}

// check periodically and turn the pump off if tank is full
void checkWaterLevel(){
  if(digitalRead(WATER_HIGH)==LOW){
    // Serial.println("Water full");
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
  lvl = "W. Level " + lvl;
  if(lvl.equals(lastLvl)==false){    
    lastLvl = lvl;
    Serial.println("last lvl "+lastLvl);
    lcdWrite(3, lvl);
  }  
  
}
