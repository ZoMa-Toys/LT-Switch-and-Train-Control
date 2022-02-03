#ifndef CallBacks_h
#define CallBacks_h

#include "switch.h"
#if defined (ESP32)
  #include "lego.h"
#endif

String helpString[20]={"DebugOff","DebugSerial","DebugWebsocket","ResetESP","SensorOff","SensorOn","CheckLights","SetThresHolds","getConfigESP","StartScan:{\"NumberOfHubs\":<NUMBER>,\"NumberOfRemotes\":<NUMBER>}","StopScan","disconnectHubs"};

void helper(){
  for (String h : helpString){
    debugPrint(h);
    delay(100);
  }
}


StaticJsonDocument<2048> emptyJSON;
void debugHandler(String action,StaticJsonDocument<2048> msg=emptyJSON){
  if (action=="DebugOff"){
    debugPrint("Debug Deactivated");
    debug = "";
  }
  else if (action=="DebugSerial"){
    debugPrint("Debug Serial Activated");
    debug = "Serial";
  }
  else if (action=="DebugWebSerial"){
    debugPrint("Debug WebSerial Activated");
    debug = "webserial";
  }
  else if (action=="DebugWebsocket"){
    debugPrint("Debug Websocket Activated");
    debug = "websocket";
  }
  else if (action=="ResetESP"){
    debugPrint("Reseting ESP");
    ESP.restart();
  }
  else if (action=="Help"){
    helper();
  }
}

void switchActions(String action,StaticJsonDocument<2048> msg=emptyJSON){
  if(action=="swtichMotor"){
      SwitchTrack(msg);
  }
  else if (action=="SensorOff"){
    debugPrint("Sensors Deactivated");
    activateSensor = false;
  }
  else if (action=="SensorOn"){
    debugPrint("Sensors Activated");
    activateSensor = true;
  }
  else if(action=="CheckLights"){
    checkLightBool=true;
    debugPrint("checkLightBool chnanged to ");
    debugPrint((checkLightBool ? "true" : "false"));
  }
  else if(action=="SetThresHolds"){
    checkLightBool=true;
    setThresholds=true;
    debugPrint("checkLightBool chnanged to ");
    debugPrint((checkLightBool ? "true" : "false"));
    debugPrint(" and setting thresholds.");
  }
  else if(action=="SwitchConfigESP:"){
    debugPrint("Call setswitches");
    SetSwitches(msg);
  }
  else if(action=="Updated:"||action=="getConfigESP"){
    messageJSONToSend["action"]="getConfigESP";
  }
}


#if defined (ESP32)
  void trainActions(String action,StaticJsonDocument<2048> msg=emptyJSON){
    if (action=="setPower"){
      setPower(msg);
    }
    else if (action=="getHubs"){
      sendHubs();
    }
    else if (action.indexOf(String("StartScan"))>-1||action=="scan"){
      if (msg.isNull()){
        DeserializationError error = deserializeJson(msg, action.substring(10,action.length()));
        if (error) {
            debugPrint("deserializeJson() failed: ");
            debugPrint(error.f_str());
        return;
        }
      }
      scan(msg);
    }
    else if (action=="stop" || action=="StopScan"){      
      ScanEnabled = false;
    }
    else if (action=="disconnectHubs"){      
      disconnectHubs();
    }
  }
#endif

void onDataReceived(String msg){
  StaticJsonDocument<2048>  messageJSON;
  DeserializationError error = deserializeJson(messageJSON, msg);
  if (error) {
      debugPrint("deserializeJson() failed: ");
      debugPrint(error.f_str());
  return;
  }

  if(messageJSON.containsKey("action")){
    debugHandler(messageJSON["action"]);
    switchActions(messageJSON["action"],messageJSON["message"]);
    #if defined (ESP32)
      trainActions(messageJSON["action"],messageJSON["message"]);
    #endif
  }
  else if(messageJSON.containsKey("Status")){
    switchActions(messageJSON["Status"],messageJSON["Message"]);
  }
}


void recvMsg(uint8_t *data, size_t len){
  WebSerial.print("Received Data:");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  debugHandler(d);
  switchActions(d);
  #if defined (ESP32)
    trainActions(d);
  #endif
}

#endif 
