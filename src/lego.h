#ifndef lego_h
#define lego_h

#include "WiFIAndWeb.h"
#include "Lpf2Hub.h"


Color TrainColors[3]={GREEN,BLUE,YELLOW};

Lpf2Hub myRemote[3];
byte portLeft = (byte)PoweredUpRemoteHubPort::LEFT;
byte portRight = (byte)PoweredUpRemoteHubPort::RIGHT;






class Hubs{
  public:
    Lpf2Hub Hub;
    int currentSpeed;
    byte MotorPort;
    Color trainColor;
    byte colorToStop;
    int distanceToStop;
    NimBLEAddress RemoteAddress;
    int RemoteID;
    byte RemotePort;
    StaticJsonDocument<200> HubJSON;
    std::__cxx11::string name;
    
    void initHubConf(){
        HubJSON["NAME"] = name;
        HubJSON["TRAIN_MOTOR"]=2;
        PortDeviceName((byte)PoweredUpHubPort::B);
        String msg;
        serializeJsonPretty(HubJSON,msg);
        debugPrint(msg);
        
      
    };
    void setTrainSpeed(int uSpeed){
      StaticJsonDocument<200> setSpeedJSON;
      StaticJsonDocument<200> MessageJSON;
      if (currentSpeed != uSpeed){
        currentSpeed = min(max(uSpeed,-100),100);
        Hub.setBasicMotorSpeed(MotorPort, currentSpeed);
        setSpeedJSON["speed"]=currentSpeed;
        setSpeedJSON["train"]=name.c_str();
        setSpeedJSON["MotorPort"]=MotorPort;
        MessageJSON["Message"]=setSpeedJSON;
        MessageJSON["Status"]="Setting Speed...";
        messageJSONToSend=MessageJSON;
      }
      debugPrint("Current speed:" + String(currentSpeed));
    }
  private:
    void PortDeviceName(byte port){
      int devtype = Hub.getDeviceTypeForPortNumber(port);
      debugPrint(String(devtype));
      if (devtype==2){
        HubJSON["TRAIN_MOTOR"]=port;
        MotorPort=port;
      }
      else if (devtype==8){
        HubJSON["LIGHT"]=port;
      }
      else if (devtype==37){
        HubJSON["COLOR_DISTANCE_SENSOR"]=port;
      }
    }

};

Hubs myHubs[3];



int remoteClick(ButtonState buttonState,int cSpeed){
      debugPrint("Buttonstate: " + (byte)buttonState);
      int uSpeed;
      if (buttonState == ButtonState::UP){
        uSpeed = min(100, cSpeed + 10);
      }
      else if (buttonState == ButtonState::DOWN){
        uSpeed = max(-100, cSpeed - 10);
      }
      else if (buttonState == ButtonState::STOP){
        uSpeed = 0;
      }
      else{
        uSpeed = cSpeed;
      }
      return uSpeed;
}

void remoteCallback(void *hub, byte portNumber, DeviceType deviceType, uint8_t *pData){
  Lpf2Hub *myRemoteHub = (Lpf2Hub *)hub;
  int thisHubID;
  for (int i = 0;i<NumberOfHubs;i++){
      if(myHubs[i].RemoteAddress == myRemoteHub->getHubAddress() && myHubs[i].RemotePort == portNumber){
          thisHubID = i;
      }
  }
  debugPrint("sensorMessage callback for port: " + String(portNumber));
  if (deviceType == DeviceType::REMOTE_CONTROL_BUTTON){
    ButtonState buttonState = myRemoteHub->parseRemoteButton(pData);
    myHubs[thisHubID].setTrainSpeed(remoteClick(buttonState,myHubs[thisHubID].currentSpeed));
  }
}

void colorDistanceSensorCallback(void *hub, byte portNumber, DeviceType deviceType, uint8_t *pData){
  Lpf2Hub *myHub = (Lpf2Hub *)hub;

  Serial.print("sensorMessage callback for port: ");
  Serial.println(portNumber, DEC);
  if (deviceType == DeviceType::COLOR_DISTANCE_SENSOR){
    int CurrentColor = myHub->parseColor(pData);
    String msg = LegoinoCommon::ColorStringFromColor(CurrentColor).c_str();
    debugPrint("Color: " + msg);
    delay(55);
    int CurrentDistance = myHub->parseDistance(pData);
    debugPrint("CurrentDistance: " + String(CurrentDistance));

    int StopColor;
    int StopDistance;
    // set hub LED color to detected color of sensor and set motor speed dependent on color
    for (int i = 0 ; i<NumberOfHubs;i++){
      if (myHubs[i].name == myHub->getHubName()){
        StopColor = (int)myHubs[i].colorToStop;
        StopDistance = (int)myHubs[i].distanceToStop;
        if (CurrentColor == StopColor || CurrentDistance == StopDistance){
          myHubs[i].setTrainSpeed(0);
        }
      }
    }
  }
}



void sendHubs(){
  StaticJsonDocument<200> MessageJSON;
  MessageJSON["Status"]="Connected Hubs:";
  JsonArray Message = MessageJSON.createNestedArray("Message");
  for (int i = 0 ; i<NumberOfHubs;i++){
    myHubs[i].initHubConf();
    Message.add(myHubs[i].HubJSON);
  }
  messageJSONToSend=MessageJSON;
}




int connectedHubs =0;
int connectedRemotes =0;

void setUpConnections(){
  for (int i = 0 ; i<NumberOfHubs;i++){
    if (!myHubs[i].Hub.isConnected()){
      debugPrint("InitHUB "+ String(i));
      myHubs[i].Hub.init();
    }
    if (myHubs[i].Hub.isConnecting()){
      if (myHubs[i].Hub.getHubType() == HubType::POWERED_UP_HUB){
        myHubs[i].Hub.connectHub();
        myHubs[i].Hub.setLedColor(LIGHTBLUE);
        connectedHubs++;
        debugPrint(String(connectedHubs) + " powered up hub(s) connected.");
      }
    }
  }
  for (int i = 0 ; i<NumberOfRemotes;i++){
    if (!myRemote[i].isConnected()){
      myRemote[i].init();
      debugPrint("InitRemote "+ String(i));
    }
    if (myRemote[i].isConnecting()){
        if (myRemote[i].getHubType() == HubType::POWERED_UP_REMOTE){
        //This is the right device
        if (!myRemote[i].connectHub()){
            debugPrint("Unable to connect to hub");
        }
        else{
            myRemote[i].setLedColor(LIGHTBLUE);
            connectedRemotes++;
            debugPrint(String(connectedRemotes) + " remote(s) connected.");

        }
        }
    }
  }
}





void PairTrainsRemote(){
  if (connectedRemotes == NumberOfRemotes && connectedHubs==NumberOfHubs){
    for (int i = 0 ; i<NumberOfHubs;i++){
        delay(100);
        myHubs[i].Hub.setLedColor(TrainColors[i]);
        myHubs[i].trainColor=TrainColors[i];
        myHubs[i].name=myHubs[i].Hub.getHubName();
        myHubs[i].initHubConf();
        byte portB = (byte)PoweredUpHubPort::B;
        if (myHubs[i].HubJSON.containsKey("COLOR_DISTANCE_SENSOR"))  myHubs[i].Hub.activatePortDevice(portB, colorDistanceSensorCallback);
        if (i<NumberOfRemotes){
            myHubs[i].RemoteID=i;
            myHubs[i].RemotePort=portLeft;
            myRemote[i].setLedColor(TrainColors[i]);
        }
        else{
            myHubs[i].RemoteID=i-NumberOfRemotes;
            myHubs[i].RemotePort=portRight;
        }
        if (NumberOfRemotes>0){
            myHubs[i].RemoteAddress = myRemote[myHubs[i].RemoteID].getHubAddress();
            myRemote[myHubs[i].RemoteID].activatePortDevice(myHubs[i].RemotePort, remoteCallback);
        }
    }
    sendHubs();
    isInitialized=true;
    debugPrint("System is initialized");
  }
}

void setPower(StaticJsonDocument<2048> messageJSON){
  for (int i = 0 ; i<NumberOfHubs;i++){
    if (myHubs[i].name==messageJSON["train"].as<std::__cxx11::string>()){
      myHubs[i].colorToStop= messageJSON["color"].as<byte>();
      myHubs[i].distanceToStop= messageJSON["distance"].as<int>();
      myHubs[i].setTrainSpeed(messageJSON["speed"].as<int>());
    }
  }
}

void scan(StaticJsonDocument<2048> messageJSON){
  ScanEnabled = true;
  NumberOfHubs = messageJSON['NumberOfHubs'].as<int>();
  NumberOfRemotes = messageJSON['NumberOfRemotes'].as<int>();
  debugPrint("ScanEnabled: " + String(ScanEnabled) );
}

void disconnectHubs(){
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

#endif