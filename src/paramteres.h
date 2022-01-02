#ifndef parameters_h
#define parameters_h


#include <WebSerial.h>
#include "Mux.h"



// create a hub instance


typedef struct {
    int pin;
    int low;
    int high;
} sensors; 


using namespace admux;
Mux mux(Pin(35, INPUT, PinType::Analog), Pinset(27, 26, 25)); 
uint8_t servo[8] = {0,1,2,3,4,5,6,7};
sensors sensor[8] = {{0,500,2500},{1,500,2500},{2,500,2500},{3,500,2500},{7,500,2500},{5,500,2500},{6,500,2500},{4,500,2500}};
const long interval = 500; 
int NumOFSwitches=0;
int NumberOfHubs = 2;
int NumberOfRemotes = 2;

bool checkLightBool = true;
bool setThresholds = true;
bool activateSensor = true;
bool debug = false;
bool ScanEnabled = false;
bool isInitialized = false;

const char* ssid = "Guber-Kray";
const char* password = "Hafnium1985!";
/* char* HOST = "guberkray.myftp.org"; */
char* HOST = "192.168.1.88";
uint16_t PORT = 80;



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
    debug = false;
  }
  else if (d.indexOf(String("DebugOn"))>-1){
    WebSerial.println("Debug Activated");
    debug = true;
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



#endif