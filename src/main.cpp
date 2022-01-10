#include "WiFIAndWeb.h"



void setup() {
  Serial.begin(115200);
  // Connect to wifi
  connectWifi();
  connectWS();
  createWebSerial();
  createOTA();
  setupPWM();
  messageJSONToSend["action"]="getConfigESP";
}

void loop() {
  // let the websockets client check for incoming messages
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