#define SPRINKLERS_ON 101
#define SPRINKLERS_OFF 102
#define INLET_OPEN 103
#define INLET_CLOSED 104
bool deviceConnected = false;
bool oldDeviceConnected = false;

BLECharacteristic *pStatusCharacteristic = NULL;
BLECharacteristic *pLogCharacteristic = NULL;
BLECharacteristic *pCommandCharacteristic = NULL;

class BTServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    Serial.println("BLE App connected");
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    Serial.println("BLE App disconnected");
    deviceConnected = false;
    pServer->startAdvertising();
  }
};

class CommandCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue();

    if (value.length() > 0) {
      debugLog("Received command: "+value);
      int tmpCmd = value.toInt();
      switch(tmpCmd){
        case SPRINKLERS_ON:
          manualWatering = 2;
          selectedActiveOption = 0;
          selectedActiveOptParam = 2;
          runActiveOptionFlag = true;
          break;
        case SPRINKLERS_OFF:
          manualWatering = 1;
          selectedActiveOption = 0;
          selectedActiveOptParam = 1;
          runActiveOptionFlag = true;
          break;
        case INLET_OPEN:
          manualFilling = 2;
          selectedActiveOption = 1;
          selectedActiveOptParam = 2;
          runActiveOptionFlag = true;
          break;
        case INLET_CLOSED:
          manualFilling = 1;
          selectedActiveOption = 1;
          selectedActiveOptParam = 1;
          runActiveOptionFlag = true;
          break;
      }
    }
  }
};

void bleSetup(){
  BLEDevice::init(BT_DEVICE_NAME);
  BLEDevice::setMTU(128);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BTServerCallbacks());

  BLEService *pService = pServer->createService(BLE_SERVICE_UUID);
  pStatusCharacteristic = pService->createCharacteristic(BLE_CHARACTEREISTIC_STATUS_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pStatusCharacteristic->addDescriptor(new BLE2902());

  pLogCharacteristic = pService->createCharacteristic(BLE_CHARACTEREISTIC_LOGS_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

  pCommandCharacteristic = pService->createCharacteristic(BLE_CHARACTEREISTIC_COMMANDS_UUID, BLECharacteristic::PROPERTY_WRITE );
  pCommandCharacteristic->setCallbacks(new CommandCallback());

  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  debugLog("BLE Charactteristics defined!");
}

bool isBTConnected(){
  return deviceConnected;
}

void writeBTStatus(String status){
  if(pStatusCharacteristic != NULL){
    pStatusCharacteristic->setValue(status);
    pStatusCharacteristic->notify();
  }
}

void writeBTLog(char* log){
  if(pLogCharacteristic != NULL){
    pLogCharacteristic->setValue(log);
    pLogCharacteristic->notify();
  }
}

