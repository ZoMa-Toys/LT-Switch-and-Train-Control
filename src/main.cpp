#include "switch.h"
#include "lego.h"

Switches switches[15];

void onDataReceived(String &data)
{
  DynamicJsonDocument jsoninput(2048);
  DeserializationError error = deserializeJson(jsoninput, data);
  if (error) {
    WebSerial.print(F("deserializeJson() failed: "));
    WebSerial.println(error.f_str());
    return;
  }
  if (jsoninput.containsKey("motor")){   
    switches[jsoninput["motor"].as<int>()].printValues();
    switches[jsoninput["motor"].as<int>()].setPulse(jsoninput["pulse"].as<int>(),jsoninput["printed"].as<bool>());
    switches[jsoninput["motor"].as<int>()].printValues();
  }
  else if (jsoninput.containsKey("Status")){
    if (jsoninput["Status"] == "SwitchConfigESP:"){
      JsonArray message = jsoninput["Message"].as<JsonArray>();
      int i = 0;
      for (JsonVariant swconf : message) {
        int midPulse = (swconf["pulse"]["Straight"].as<int>() + swconf["pulse"]["Turn"].as<int>())/2;
        switches[i].setParameters(i,swconf["pulse"][swconf["switched"].as<String>()].as<int>(),swconf["printed"].as<String>(),midPulse);
        switches[i].printValues();
        i++;
      }
      NumOFSwitches = i;
    }
  }
  else if (jsoninput.containsKey("action")){
    if (jsoninput["action"]=="setPower"){
      for (int i = 0 ; i<NumberOfHubs;i++){
        if (myHubs[i].name==jsoninput["train"].as<std::__cxx11::string>()){
          myHubs[i].colorToStop= jsoninput["color"].as<byte>();
          myHubs[i].distanceToStop= jsoninput["distance"].as<int>();
          myHubs[i].setTrainSpeed(jsoninput["speed"].as<int>());
        }
      }
    }
    else if (jsoninput["action"]=="getHubs"){
      sendHubs();
    }
    else if (jsoninput["action"]=="scan"){      
      ScanEnabled = true;
      NumberOfHubs = jsoninput['NumberOfHubs'].as<int>();
      NumberOfRemotes = jsoninput['NumberOfRemotes'].as<int>();
    }
    else if (jsoninput["action"]=="stop"){      
      ScanEnabled = false;
    }
    else if (jsoninput["action"]=="disconnectHubs"){      
      ScanEnabled = false;
      for (int i = 0 ; i<NumberOfHubs;i++){
        myHubs[i].Hub.shutDownHub();
      }
      for (int i = 0 ; i<NumberOfRemotes;i++){
        myRemote[i].shutDownHub();
      }
      delete[] myHubs;
      delete[] myRemote;
      Lpf2Hub myRemote[3];
      Hubs myHubs[3];
    }
  }
}


void GetData(WebSocketClient &wsc,WiFiClient &wifiClient){
  String data;
 
  if (wifiClient.connected()) {
    wsc.getData(data);
    if (data.length() > 0) {
      onDataReceived(data);
    }
  } else {
    WebSerial.println("Client disconnected. (get)");
    wifiClient.connect(HOST, PORT);
  }
};

void setupFunc(){
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

/*   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32.");
  }); */
  WebSerial.begin(&server);
    /* Attach Message Callback */
  WebSerial.msgCallback(recvMsg);
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  WebSerial.println("HTTP server started");
  if(initWebSocket("/switch",webSocketClientSwitch, clientSwitch) == 0) { return ; };
  if(initWebSocket("/train",webSocketClientTrain, clientTrain) == 0) { return ; };  
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);
  StaticJsonDocument<200> getConfigESPJSON;
  String getConfigESP;
  getConfigESPJSON["action"]="getConfigESP";
  serializeJson(getConfigESPJSON,getConfigESP);
  dataToSend = getConfigESP;

  SendData(webSocketClientTrain,clientTrain,dataToSend);

/*   myRemote.init(); // initialize the remote hub
  
  for (int i = 0 ; i<NumberOfHubs;i++){
    myHubs[i].Hub.init();    // initialize the listening hub
  } */
}



void setup(void) {
  setupFunc();
}

bool firstBoot = true;

void loop(void) {
  if (firstBoot){
    checkLightBool=true;
    setThresholds=true;
    firstBoot=false;
    delay(1111);
  }
  GetData(webSocketClientSwitch,clientSwitch);
  GetData(webSocketClientTrain,clientTrain);
  for (int i = 0; i < NumOFSwitches ; i++){
    dataToSend = switches[i].getSensorStatus();
    if(dataToSend.length() > 0){
      SendData(webSocketClientSwitch,clientSwitch,dataToSend);
    }
  }
  if(checkLightBool){
    delay(2222);
    CheckLights();
  }
  if (ScanEnabled){
    delay(100);
    setUpConnections();
  }
  if (!isInitialized){
    PairTrainsRemote();
  }
}
