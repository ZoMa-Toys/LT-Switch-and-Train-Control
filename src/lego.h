#ifndef lego_h
#define lego_h

#include "WifiAndWeb.h"
#include "Lpf2Hub.h"


Color TrainColors[3]={GREEN,YELLOW,BLUE};

Lpf2Hub myRemote[3];
byte portLeft = (byte)PoweredUpRemoteHubPort::LEFT;
byte portRight = (byte)PoweredUpRemoteHubPort::RIGHT;

int NumberOfHubs = 1;
int NumberOfRemotes = 1;


bool ScanEnabled = false;
bool isInitialized = false;




class Hubs{
  public:
    Lpf2Hub Hub;
    int currentSpeed;
    int currentLight;
    byte MotorPort = 0;
    byte LightPort = 1;
    Color trainColor;
    byte colorToStop=255;
    int distanceToStop;
    byte colorToSlow=255;
    int distanceToSlow;
    NimBLEAddress RemoteAddress;
    int RemoteID;
    byte RemotePort;
    StaticJsonDocument<500> HubJSON;
    std::__cxx11::string name;
    
    void initHubConf(){
        HubJSON["NAME"] = name;
        HubJSON["TRAIN_MOTOR"]=0;
        HubJSON["traincolor"]=trainColor;
        PortDeviceName((byte)PoweredUpHubPort::B);

        
      
    };
    void setTrainSpeed(int uSpeed){
      if (currentSpeed != uSpeed){
        currentSpeed = min(max(uSpeed,-100),100);
        Hub.setBasicMotorSpeed(MotorPort, currentSpeed);
        messageJSONToSend["Status"]="Setting Speed...";
        JsonObject message = messageJSONToSend.createNestedObject("Message");
        message["speed"]=currentSpeed;
        message["train"]=name.c_str();
        message["color"]=colorToStop;
        message["colorSlow"]=colorToSlow;
        message["distance"]=distanceToStop;
        message["distanceSlow"]=distanceToSlow;
      }
      debugPrint("Current speed:" + String(currentSpeed));
    }
    void setTrainLight(int uLight){
      if (currentLight != uLight){
        currentLight = min(max(uLight,-100),100);
        Hub.setBasicMotorSpeed(LightPort, currentLight);
        messageJSONToSend["Status"]="Setting Speed...";
        JsonObject message = messageJSONToSend.createNestedObject("Message");
        message["light"]=currentLight;
        message["train"]=name.c_str();
      }
      debugPrint("Current light:" + String(currentLight));
    }
  private:
    void PortDeviceName(byte port){
      int devtype = Hub.getDeviceTypeForPortNumber(port);
      delay(300);
      debugPrint("DEVICE TYPE:" + String(devtype));
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
    int SlowColor;
    int SlowDistance;
    // set hub LED color to detected color of sensor and set motor speed dependent on color
    for (int i = 0 ; i<NumberOfHubs;i++){
      if (myHubs[i].name == myHub->getHubName()){
        StopColor = (int)myHubs[i].colorToStop;
        StopDistance = (int)myHubs[i].distanceToStop;
        SlowColor = (int)myHubs[i].colorToSlow;
        SlowDistance = (int)myHubs[i].distanceToSlow;
        if (CurrentColor == StopColor || CurrentDistance == StopDistance){
          myHubs[i].setTrainSpeed(0);
        }
        if (CurrentColor == SlowColor || CurrentDistance == SlowDistance){
          myHubs[i].setTrainSpeed(20);
        }
    }
    }
  }
}



void sendHubs(){
  messageJSONToSend["Status"]="Connected Hubs:";
  JsonArray Message = messageJSONToSend.createNestedArray("Message");
  for (int i = 0 ; i<NumberOfHubs;i++){
    myHubs[i].initHubConf();
    Message.add(myHubs[i].HubJSON);
  }
}

String typeIntToString[8]={"UNKNOWNHUB","MISSING","BOOST_MOVE_HUB","POWERED_UP_HUB","POWERED_UP_REMOTE","DUPLO_TRAIN_HUB","CONTROL_PLUS_HUB","MARIO_HUB"};


int connectedHubs =0;
int connectedRemotes =0;

void connectHUB(Lpf2Hub &hub,HubType type){
  if (!hub.isConnected()){
      hub.init();
      delay(500);
      debugPrint("Init " + typeIntToString[int(type)]);
    }
  if (hub.isConnecting()){
    if (hub.getHubType() == type){
      hub.connectHub();
      debugPrint(typeIntToString[int(type)] + " is connecting");
      if (hub.isConnected()){
        debugPrint("Connected to " + typeIntToString[int(type)]);
        hub.setLedColor(LIGHTBLUE);
        if (type == HubType::POWERED_UP_HUB ){
          connectedHubs++;
        }
        else if (type == HubType::POWERED_UP_REMOTE ){
          connectedRemotes++;
        }
      }
      else{
        debugPrint("Failed to connect to " + typeIntToString[int(type)] );
      }
    }
  }
}

void setUpConnections(){
  for (int i = 0 ; i<NumberOfHubs;i++){
    connectHUB(myHubs[i].Hub,HubType::POWERED_UP_HUB);
  }
  for (int i = 0 ; i<NumberOfRemotes;i++){
    connectHUB(myRemote[i],HubType::POWERED_UP_REMOTE);
  }
}

void PairTrainsRemote(){
  if (connectedRemotes == NumberOfRemotes && connectedHubs==NumberOfHubs){
    for (int i = 0 ; i<NumberOfHubs;i++){
      delay(100);
        debugPrint("color: " + String(TrainColors[i]));
        myHubs[i].Hub.setLedColor(TrainColors[i]);
        delay(200);
        myHubs[i].trainColor=TrainColors[i];
        myHubs[i].name=myHubs[i].Hub.getHubName();
        delay(200);
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
    ScanEnabled=false;
    debugPrint("System is initialized"); 
  }
}

void setPower(StaticJsonDocument<2048> messageJSON){
  for (int i = 0 ; i<NumberOfHubs;i++){
    if (myHubs[i].name==messageJSON["train"].as<std::__cxx11::string>()){
      myHubs[i].colorToStop= (messageJSON["color"].as<byte>()==255?1000:messageJSON["color"].as<byte>());
      myHubs[i].distanceToStop= messageJSON["distance"].as<int>();
      myHubs[i].colorToStop= (messageJSON["colorSlow"].as<byte>()==255?1000:messageJSON["colorSlow"].as<byte>());
      myHubs[i].distanceToStop= messageJSON["distanceSlow"].as<int>();
      myHubs[i].setTrainSpeed(messageJSON["speed"].as<int>());
      myHubs[i].setTrainLight(messageJSON["light"].as<int>());
    }
  }
}

void scan(StaticJsonDocument<2048> msg){
  ScanEnabled = true;
  NumberOfHubs = msg["NumberOfHubs"].as<int>();
  NumberOfRemotes = msg["NumberOfRemotes"].as<int>();
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
  Lpf2Hub myRemote[3];
  Hubs myHubs[3];
}

#endif
