#ifndef CallBacks_h
#define CallBacks_h

#include "WifiAndWeb.h"
#include "switch.h"
#include "lego.h"


void onDataReceived(String msg){
  StaticJsonDocument<2048>  messageJSON;
  DeserializationError error = deserializeJson(messageJSON, msg);
  if (error) {
      debugPrint("deserializeJson() failed: ");
      debugPrint(error.f_str());
  return;
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