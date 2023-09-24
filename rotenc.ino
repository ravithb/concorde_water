static int64_t rotSwDbT = 0;



void onRotSw(){
  //debounce
  if((millis() - rotSwDbT) < 500) return;

  rotSwDbT = millis();

  updateScreen = true;
  if(!isBacklightOn()){
    backlightOnFlag = true;  
    //displayStatus();  
  }else{
    menuInterrupt = true;
    triggerMenu();
  }
  // Serial.println("SW");
}

// // on left or right rotattion
// void showDirection(ESPRotary& r) {
//   Serial.println(r.directionToString(r.getDirection()));
// }

// // on change
// void rotate(ESPRotary& r) {
//    Serial.println(r.getPosition());
// }