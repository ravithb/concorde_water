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
}
