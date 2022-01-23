#include "CallBacks.h"



void setup() {
  Serial.begin(115200);
  // Connect to wifi
  connectWifi();
  connectWS(onDataReceived);
  createWebSerial(recvMsg);
  setupPWM();
  messageJSONToSend["action"]="getConfigESP";
  createOTA();
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
    for (int i = 0; i < NumOFSwitches ; i++){
      switches[i].getSensorStatus();

    }
    if(checkLightBool){
      CheckLights();
    }
    if (ScanEnabled){
      setUpConnections();
    }
    if (!isInitialized){
      PairTrainsRemote();
    }
  }
  else{
    client.connect(websockets_server_host, websockets_server_port, websockets_server_path);
  }
  

}