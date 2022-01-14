#ifndef WiFIAndWeb_h
#define WiFIAndWeb_h

#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebSerial.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "Mux.h"

using namespace websockets;
using namespace admux;

const char* ssid = "Guber-Kray"; //Enter SSID
const char* password = "Hafnium1985!"; //Enter Password
const char* websockets_server_host = "192.168.1.88"; //Enter server adress
const uint16_t websockets_server_port = 80; // Enter server port
const char* websockets_server_path = "/ws"; //Enter server adress
String debug = "";
StaticJsonDocument<2048>  messageJSONToSend;

typedef struct {
    int pin;
    int low;
    int high;
} sensors; 

Mux mux(Pin(35, INPUT, PinType::Analog), Pinset(27, 26, 25)); 
uint8_t servo[8] = {0,1,2,3,4,5,6,7};
sensors sensor[8] = {{0,500,2500},{1,500,2500},{2,500,2500},{3,500,2500},{7,500,2500},{5,500,2500},{6,500,2500},{4,500,2500}};
const long interval = 500; 
int NumOFSwitches=0;
int NumberOfHubs = 1;
int NumberOfRemotes = 1;

bool checkLightBool = true;
bool setThresholds = true;
bool activateSensor = true;
bool ScanEnabled = false;
bool isInitialized = false;



WebsocketsClient client;
AsyncWebServer server(80);

void debugPrint(String toprint){
    if (debug == "Serial"){
        Serial.println(toprint);
    }
    else if (debug == "webserial"){
        WebSerial.println(toprint);
    }
    else if (debug == "websocket"){
        StaticJsonDocument<2048> tp;
        tp["DEBUG"]=toprint;
        String message;
        serializeJson(tp,message);
        client.send(message);
    }
}

void sendJSON(){
    String toSend = "";
    serializeJson(messageJSONToSend,toSend);
    messageJSONToSend.clear();
    debugPrint("toSend: " + toSend);
    client.send(toSend);
    toSend = "";

}

#include "switch.h"
#include "lego.h"


void onDataReceived(String msg){
  debugPrint("Incoming WS msg: " + msg);
  StaticJsonDocument<2048>  messageJSON;
  if (msg.indexOf("TrackConfig")==-1 && msg.indexOf("CardMap")==-1){
      DeserializationError error = deserializeJson(messageJSON, msg);
      if (error) {
          debugPrint("deserializeJson() failed: ");
          debugPrint(error.f_str());
      return;
      }
  }

  if(messageJSON.containsKey("action")){
    if(messageJSON["action"]=="swtichMotor"){
        SwitchTrack(messageJSON["message"]);
    }
    else if (messageJSON["action"]=="setPower"){
      setPower(messageJSON["message"]);
    }
    else if (messageJSON["action"]=="getHubs"){
      sendHubs();
    }
    else if (messageJSON["action"]=="scan"){      
      scan(messageJSON);
    }
    else if (messageJSON["action"]=="stop"){      
      ScanEnabled = false;
    }
    else if (messageJSON["action"]=="disconnectHubs"){      
      disconnectHubs();
    }
    else if (messageJSON["action"]=="SetThresHolds"){      
      checkLightBool=true;
      setThresholds=true;
    }
  }
  else if(messageJSON.containsKey("Status")){
      if(messageJSON["Status"]=="SwitchConfigESP:"){
          debugPrint("Call setswitches");
          SetSwitches(messageJSON["Message"]);
      }
      else if(messageJSON["Status"]=="Updated:"){
          messageJSONToSend["action"]="getConfigESP";
      }
  }
}


void connectWifi(){
    WiFi.begin(ssid, password);

    // Wait some time to connect to wifi
    Serial.println("connecting to WiFi");
    for(int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
        Serial.print(".");
        delay(1000);
    }

    // Check if connected to wifi
    if(WiFi.status() != WL_CONNECTED) {
        Serial.println("No Wifi!");
        return;
    }
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void connectWS(){
    Serial.println("Connecting to server.");
    // try to connect to Websockets server
    bool connected = client.connect(websockets_server_host, websockets_server_port, websockets_server_path);
    if(connected) {
        Serial.println("Connected!");
        client.send("Hello Server");
    } else {
        Serial.println("Not Connected!");
    }
    client.onMessage([&](WebsocketsMessage message){
        String msg;
        msg=message.data();
        onDataReceived(msg);
    });
}

void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  if (d.indexOf(String("SensorOff"))>-1){
    WebSerial.println("Sensors Deactivated");
    activateSensor = false;
  }
  else if (d.indexOf(String("SensorOn"))>-1){
    WebSerial.println("Sensors Activated");
    activateSensor = true;
  }
  else if (d.indexOf(String("DebugOff"))>-1){
    WebSerial.println("Debug Deactivated");
    debug = "";
  }
  else if (d.indexOf(String("DebugOn"))>-1){
    WebSerial.println("Debug Activated");
    debug = "webserial";
  }
/*   else if(d.indexOf(String("action"))>-1){
    SendData(webSocketClientTrain,clientTrain,d);
  }
  else if(d.indexOf(String("motor"))>-1){
    SendData(webSocketClientSwitch,clientSwitch,d);
  } */
  else if(d.indexOf(String("CheckLights"))>-1){
    checkLightBool=true;
    WebSerial.print("checkLightBool chnanged to ");
    WebSerial.println(checkLightBool);
  }
  else if(d.indexOf(String("SetThresHolds"))>-1){
    checkLightBool=true;
    setThresholds=true;
    WebSerial.print("checkLightBool chnanged to ");
    WebSerial.print(checkLightBool);
    WebSerial.println(" and setting thresholds.");
  }
  /* else if(d.indexOf(String("Reconnect"))>-1){
    if(initWebSocket("/switch",webSocketClientSwitch, clientSwitch) == 0) { return ; };
    if(initWebSocket("/train",webSocketClientTrain, clientTrain) == 0) { return ; };  
  } */
  else if (d.indexOf(String("StartScan"))>-1){
    WebSerial.println("Scan Started");
    ScanEnabled = true;
  }
  else if (d.indexOf(String("StopScan"))>-1){
    WebSerial.println("Scan Stoped");
    ScanEnabled = false;
  }
  else if (d.indexOf(String("ResetESP"))>-1){
    WebSerial.println("Reseting ESP");
    ESP.restart();
  }
/*   else if (d.indexOf(String("SendHubs"))>-1){
    WebSerial.println("Hubs Sent");
    sendHubs();
  } */
  else if (d.indexOf(String("NumberOfHubs"))>-1){
    NumberOfHubs = d.substring(13,d.length()).toInt();
    WebSerial.print("Setting Number Of Hubs To ");
    WebSerial.println(NumberOfHubs);
  }
  else if (d.indexOf(String("NumberOfRemotes"))>-1){
    NumberOfRemotes = d.substring(16,d.length()).toInt();
    WebSerial.print("Setting Number Of Remotes To ");
    WebSerial.println(NumberOfRemotes);
  }
  else if (d.indexOf(String("Help"))>-1){
    WebSerial.println("SensorOff");
    WebSerial.println("SensorOn");
    WebSerial.println("StartScan");
    WebSerial.println("StopScan");
    /* WebSerial.println("SendHubs"); */
    WebSerial.println("NumberOfHubs:<NUMBER>");
    WebSerial.println("NumberOfRemotes:<NUMBER>");
    WebSerial.println("DebugOff");
    WebSerial.println("DebugOn");
    WebSerial.println("CheckLights");
    WebSerial.println("SetThresHolds");
    WebSerial.println("ResetESP");
  }
  WebSerial.println(d);
}

void createWebSerial(){
  WebSerial.begin(&server);
    /* Attach Message Callback */
  WebSerial.msgCallback(recvMsg);
}

void createOTA(){
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
}




#endif 