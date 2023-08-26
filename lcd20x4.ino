int busy =0;

void initLCD(){
  int error;
  Serial.println("Probing for PCF8574 on address 0x27...");

  // See http://playground.arduino.cc/Main/I2cScanner how to test for a I2C device.
  Wire.begin();
  Wire.beginTransmission(0x27);
  error = Wire.endTransmission();
  Serial.print("Error: ");
  Serial.print(error);

  if (error == 0) {
    Serial.println(": LCD found.");
    lcd.begin(20, 4);  // initialize the lcd
    lcd.setBacklight(255);
  } else {
    Serial.println(": LCD not found.");
  }  
  
  
}

void testLCD(){
  lcd.setBacklight(255);
  lcd.home();
  lcd.clear();
  lcd.print("Welcome");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("*** first line.");
  lcd.setCursor(0, 1);
  lcd.print("*** second line.");
  lcd.setCursor(0, 2);
  lcd.print("*** third line.");
  lcd.setCursor(0, 3);
  lcd.print("*** fourth line.");
  for(int i=0;i<20;i++){
    lcd.scrollDisplayLeft();
    delay(300);
  }
  for(int i=0;i<20;i++){
    lcd.scrollDisplayRight();
    delay(300);
  }
  lcd.setBacklight(0);
  delay(1000);
  lcd.setBacklight(255);
  lcd.clear();
  lcd.print("TEST COMPLETE");
  delay(2000);

}

void clearLine(int row){
  lcd.setCursor(0,row);
  lcd.print("                    ");
  lcd.setCursor(0,row);
}

void lcdWrite(int row, String msg){
  if(isInSettings==0){
    clearLine(row);
    lcd.print(msg);
  }
}
