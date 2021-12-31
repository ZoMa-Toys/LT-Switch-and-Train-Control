#ifndef lego_h
#define lego_h

#include "paramteres.h"
#include <webAndWifi.h>

class Hubs{
  public:
    Lpf2Hub Hub;
    int currentSpeed;
    byte MotorPort;
    Color trainColor;
    byte colorToStop;
    int distanceToStop;
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

void colorDistanceSensorCallback(void *hub, byte portNumber, DeviceType deviceType, uint8_t *pData){
    Lpf2Hub *myHub = (Lpf2Hub *)hub;

    Serial.print("sensorMessage callback for port: ");
    Serial.println(portNumber, DEC);
    if (deviceType == DeviceType::COLOR_DISTANCE_SENSOR)
    {
        int CurrentColor = myHub->parseColor(pData);
        if (debug) WebSerial.print("Color: ");
        if (debug) WebSerial.println(LegoinoCommon::ColorStringFromColor(CurrentColor).c_str());
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
            }
        }

        if (CurrentColor == StopColor || CurrentDistance == StopDistance)
        {
            myHub->stopBasicMotor((byte)PoweredUpHubPort::A);
        }/* 
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
  if (debug) WebSerial.print("sensorMessage callback for port: ");
  if (debug) WebSerial.println(portNumber);
  if (deviceType == DeviceType::REMOTE_CONTROL_BUTTON){
    ButtonState buttonState = myRemoteHub->parseRemoteButton(pData);
    if (portNumber == portLeft){
       myHubs[0].setTrainSpeed(remoteClick(buttonState,myHubs[0].currentSpeed));
    }
    if (NumberOfHubs>1){
      if (portNumber == portRight){
        myHubs[1].setTrainSpeed(remoteClick(buttonState,myHubs[1].currentSpeed));
      }
    }
  }
}





void setUpConnections(){
  for (int i = 0 ; i<NumberOfHubs;i++){
    if (myHubs[i].Hub.isConnecting()){
      if (myHubs[i].Hub.getHubType() == HubType::POWERED_UP_HUB){
        myHubs[i].Hub.connectHub();
        myHubs[i].Hub.setLedColor(LIGHTBLUE);
        if (debug) WebSerial.println("powered up hub connected.");
      }
    }
    if (!myHubs[i].Hub.isConnected()){
      myHubs[i].Hub.init();
    }
  }
  if (myRemote.isConnecting()){
    if (myRemote.getHubType() == HubType::POWERED_UP_REMOTE){
      //This is the right device
      if (!myRemote.connectHub()){
        if (debug) WebSerial.println("Unable to connect to hub");
      }
      else{
        myRemote.setLedColor(LIGHTBLUE);
        if (debug) WebSerial.println("Remote connected.");
      }
    }
  }
  if (!myRemote.isConnected()){
    myRemote.init();
  }
}





void PairTrainsRemote(){
  /* isInitialized = myRemote.isConnected(); */
  int connectedHubs =0;
  for (int i = 0 ; i<NumberOfHubs;i++){
    if (myHubs[i].Hub.isConnected()) connectedHubs++;
/*     if (isInitialized && myHubs[i].Hub.isConnected()){
      isInitialized=true;
    }
    else{
      isInitialized=false;
    } */
  }

/*   if (debug) WebSerial.print(connectedHubs);  
  if (debug) WebSerial.print("/");  
  if (debug) WebSerial.println(NumberOfHubs); */

/* if(myRemote.isConnected() && myHubs[0].Hub.isConnected() && myHubs[1].Hub.isConnected() && !isInitialized){ */
/*     isInitialized = true; */
  delay(200); //needed because otherwise the message is to fast after the connection procedure and the message will get lost
  // both activations are needed to get status updates
  if (myRemote.isConnected() && connectedHubs==NumberOfHubs){
    if (NumberOfHubs >0) myRemote.activatePortDevice(portLeft, remoteCallback);
    if (NumberOfHubs >1) myRemote.activatePortDevice(portRight, remoteCallback);
    myRemote.setLedColor(RED);
    for (int i = 0 ; i<NumberOfHubs;i++){
      delay(100);
      myHubs[i].Hub.setLedColor(TrainColors[i]);
      myHubs[i].trainColor=TrainColors[i];
      myHubs[i].name=myHubs[i].Hub.getHubName();
      myHubs[i].initHubConf();
      byte portB = (byte)PoweredUpHubPort::B;
      myHubs[i].Hub.activatePortDevice(portB, colorDistanceSensorCallback);
    }
    sendHubs();
    isInitialized=true;
    if (debug) WebSerial.println("System is initialized with remote");
  }
  else if (connectedHubs==NumberOfHubs){
    for (int i = 0 ; i<NumberOfHubs;i++){
      delay(100);
      myHubs[i].Hub.setLedColor(TrainColors[i]);
      myHubs[i].trainColor=TrainColors[i];
      myHubs[i].name=myHubs[i].Hub.getHubName();
      myHubs[i].initHubConf();
      byte portB = (byte)PoweredUpHubPort::B;
      myHubs[i].Hub.activatePortDevice(portB, colorDistanceSensorCallback);
    }
    sendHubs();
    isInitialized=true;
    if (debug) WebSerial.println("System is initialized only Hub(s)");
  }
}

#endif