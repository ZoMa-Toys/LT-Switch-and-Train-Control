#ifndef lego_h
#define lego_h

#include "paramteres.h"
#include <webAndWifi.h>
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
        if (debug) serializeJsonPretty(HubJSON,Serial);
        
      
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
        String message;
        serializeJson(MessageJSON,message);
        if (debug) WebSerial.println(message);
        SendData(webSocketClientTrain,clientTrain,message);
      }
      if (debug) WebSerial.print("Current speed:");
      if (debug) WebSerial.println(currentSpeed);
    }
  private:
    void PortDeviceName(byte port){
      if (debug) WebSerial.println(Hub.getDeviceTypeForPortNumber(port));
      if (Hub.getDeviceTypeForPortNumber(port)==2){
        HubJSON["TRAIN_MOTOR"]=port;
        MotorPort=port;
      }
      else if (Hub.getDeviceTypeForPortNumber(port)==8){
        HubJSON["LIGHT"]=port;
      }
      else if (Hub.getDeviceTypeForPortNumber(port)==37){
        HubJSON["COLOR_DISTANCE_SENSOR"]=port;
      }
    }

};

Hubs myHubs[3];



int remoteClick(ButtonState buttonState,int cSpeed){
      if (debug) WebSerial.print("Buttonstate: ");
      if (debug) WebSerial.println((byte)buttonState);
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
  if (debug) WebSerial.print("sensorMessage callback for port: ");
  if (debug) WebSerial.println(portNumber);
  if (deviceType == DeviceType::REMOTE_CONTROL_BUTTON){
    ButtonState buttonState = myRemoteHub->parseRemoteButton(pData);
    myHubs[thisHubID].setTrainSpeed(remoteClick(buttonState,myHubs[thisHubID].currentSpeed));
  }
}

void colorDistanceSensorCallback(void *hub, byte portNumber, DeviceType deviceType, uint8_t *pData){
    Lpf2Hub *myHub = (Lpf2Hub *)hub;

    Serial.print("sensorMessage callback for port: ");
    Serial.println(portNumber, DEC);
    if (deviceType == DeviceType::COLOR_DISTANCE_SENSOR)
    {
        int CurrentColor = myHub->parseColor(pData);
        if (debug) WebSerial.print("Color: ");
        if (debug) WebSerial.println(LegoinoCommon::ColorStringFromColor(CurrentColor).c_str());
        delay(55);
        int CurrentDistance = myHub->parseDistance(pData);
        if (debug) WebSerial.print("CurrentDistance: ");
        if (debug) WebSerial.println(CurrentDistance);

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

        /* 
        else if (color == (byte)Color::YELLOW)
        {
            myHub->setLedColor((Color)color);
            myHub->setBasicMotorSpeed((byte)PoweredUpHubPort::A, 25);
        }
        else if (color == (byte)Color::BLUE)
        {
            myHub->setLedColor((Color)color);
            myHub->setBasicMotorSpeed((byte)PoweredUpHubPort::A, 35);
        } */
    }
  }



void sendHubs(){
  StaticJsonDocument<200> MessageJSON;
  MessageJSON["Status"]="Connected Hubs:";
  JsonArray Message = MessageJSON.createNestedArray("Message");
  String stringToSend;
  for (int i = 0 ; i<NumberOfHubs;i++){
    myHubs[i].initHubConf();
    Message.add(myHubs[i].HubJSON);
  }
  serializeJson(MessageJSON,stringToSend);
  SendData(webSocketClientTrain,clientTrain,stringToSend);
}




int connectedHubs =0;
int connectedRemotes =0;

void setUpConnections(){
  for (int i = 0 ; i<NumberOfHubs;i++){
    if (myHubs[i].Hub.isConnecting()){
      if (myHubs[i].Hub.getHubType() == HubType::POWERED_UP_HUB){
        myHubs[i].Hub.connectHub();
        myHubs[i].Hub.setLedColor(LIGHTBLUE);
        connectedHubs++;
        if (debug) WebSerial.print(connectedHubs);
        if (debug) WebSerial.println(" powered up hub connected.");
      }
    }
    if (!myHubs[i].Hub.isConnected()){
      myHubs[i].Hub.init();
    }
  }
  for (int i = 0 ; i<NumberOfRemotes;i++){
    if (myRemote[i].isConnecting()){
        if (myRemote[i].getHubType() == HubType::POWERED_UP_REMOTE){
        //This is the right device
        if (!myRemote[i].connectHub()){
            if (debug) WebSerial.println("Unable to connect to hub");
        }
        else{
            myRemote[i].setLedColor(LIGHTBLUE);
            connectedRemotes++;
            if (debug) WebSerial.print(connectedRemotes);
            if (debug) WebSerial.println(" Remote connected.");

        }
        }
    }
    if (!myRemote[i].isConnected()){
        myRemote[i].init();
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
    if (debug) WebSerial.println("System is initialized");
  }
}

#endif