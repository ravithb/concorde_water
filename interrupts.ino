void onWaterLow(){
  currentWaterLevel = 0;
}

void onWaterFullChange(){
  if(digitalRead(WATER_HIGH) == LOW){
    // change is from High to Low => Water level just reached the top
    currentWaterLevel = 2;
  }else{
    // change is from Low to High => Water level just went down from being full
    currentWaterLevel = 1;
  }
  
}

void onInletTimer(){
  inletTimerStatus = digitalRead(SOL_INLET);
}

void onSprinklerTimer(){
  sprinklerTimerStatus = digitalRead(SOL_SPRINKLERS);
}