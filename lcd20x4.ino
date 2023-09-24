#define CHARS 20
#define ROWS 4
#define LCD_ON_TIME 300 //seconds

int busy =0;
int backlightCounter = LCD_ON_TIME;
int backlightValue = 255;
bool backlightOnFlag = true;
bool keepBacklightOn = false;

void setKeepBacklightOn(bool bo){
  keepBacklightOn = bo;
  if(bo){
    backlightCounter = LCD_ON_TIME;
  }
}

bool isBacklightOn(){
  return backlightValue > 0;
}

void backlight(bool on){
  if(on){
    if(backlightValue == 0){
      backlightValue = 255;
      backlightCounter = LCD_ON_TIME;
    }    
  }else{
    backlightValue = 0;
    lcd.clear();
  }
  lcd.setBacklight(backlightValue);
}

void checkBacklight(){
  backlight(backlightOnFlag);
}

void autoLcdBacklightISR(){
  if(backlightOnFlag){
    if(backlightCounter <= 0) {
      backlightOnFlag = false;
      backlightCounter = 0;
    }else{
      if(!keepBacklightOn){
        backlightCounter--;
      }
    }
  }
}

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
    lcd.begin(CHARS, ROWS);  // initialize the lcd
    backlight(true);
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
  for(int n = 0; n < CHARS; n++) // 20 indicates symbols in line. For 2x16 LCD write - 16
  {
    lcd.print(" ");
  }
  lcd.setCursor(0,row);
}

void lcdWrite(int row, String msg){
  if(isInSettings==0){
    clearLine(row);
    lcd.print(msg);
  }
}
