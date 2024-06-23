
void debugLog(String s){
  Serial.println(s);
  writeBTLog((char *)s.c_str());
}

