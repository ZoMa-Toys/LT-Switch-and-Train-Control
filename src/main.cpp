#include "CallBacks.h"



void setup() {
  Serial.begin(115200);
  // Connect to wifi
  connectWifi();
  connectWS();
  createWebSerial(recvMsg);
  createOTA();
  setupPWM_Servo();
  messageJSONToSend["action"]="getConfigESP";
  #if defined (ARDUINO_ARCH_ESP8266)
    LEDSetup();
  #endif
}

void loop() {
  ArduinoOTA.handle();
  // let the websockets client check for incoming messages
  if (WiFi.status() != WL_CONNECTED){
    ESP.restart();
  }
  if(client.available()) {
    client.poll();
    if (!messageJSONToSend.isNull()){
      sendJSON();
    }
    for (int i = minswitchID; i <= min(maxswitchID,NumOFSwitches-1) ; i++){
      switches[i].getSensorStatus();
    }
    if(checkLightBool){
      CheckLights();
    }
    #if defined (ESP32)
      if (ScanEnabled){
        setUpConnections();
      }
      if (!isInitialized){
        PairTrainsRemote();
      }
    #endif
  }
  else{
    connectWS();
  }
  

}