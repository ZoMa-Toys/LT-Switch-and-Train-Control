#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <WebSocketClient.h>
#include <WebSerial.h>
#include <ArduinoJson.h>
#include <Adafruit_PWMServoDriver.h>
#include "Mux.h"
#include "Lpf2Hub.h"

using namespace admux;

typedef struct {
    int pin;
    int low;
    int high;
} sensors; 

Mux mux(Pin(35, INPUT, PinType::Analog), Pinset(27, 26, 25)); 

const char* ssid = "Guber-Kray";
const char* password = "Hafnium1985!";
char* HOST = "guberkray.myftp.org";
/* char* HOST = "192.168.1.88"; */
uint16_t PORT = 80;
uint8_t servo[8] = {0,1,2,3,4,5,6,7};
sensors sensor[8] = {{0,500,2500},{1,500,2500},{2,500,2500},{3,500,2500},{7,500,2500},{5,500,2500},{6,500,2500},{4,500,2500}};
/* int sensor[8] = {0,1,2,3,7,5,6,4}; */



const long interval = 500;  
bool activateSensor = true;
bool debug = true;
bool checkLightBool = true;
bool setThresholds = true;
int NumOFSwitches=0;
bool ScanEnabled = true;
bool isInitialized = false;

Color TrainColors[3]={GREEN,BLUE,YELLOW};

// create a hub instance
Lpf2Hub myRemote;
byte portLeft = (byte)PoweredUpRemoteHubPort::LEFT;
byte portRight = (byte)PoweredUpRemoteHubPort::RIGHT;
/* byte portA = (byte)PoweredUpHubPort::A;
Lpf2Hub myHub;
int currentSpeed = 0;
Lpf2Hub myHub2;
int currentSpeed2 = 0;
bool isInitialized = false; */


String dataToSend; // update this with the value you wish to send to the server


Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
AsyncWebServer server(80);
WebSocketClient webSocketClientSwitch;
WebSocketClient webSocketClientTrain;
WiFiClient clientSwitch;
WiFiClient clientTrain;

void SendData(WebSocketClient &wsc, WiFiClient &c ,String &dataToSend){
  if (c.connected()) {
    if(dataToSend.length() > 0)
    {
      wsc.sendData(dataToSend);
      if (debug){
        WebSerial.print("Data Sent: ");
        WebSerial.println(dataToSend);
      };
    }
    dataToSend = "";
  } else {
    WebSerial.println("Client disconnected. (send)");
    c.connect(HOST, PORT);
  }
};

int initWebSocket(char* wspath,WebSocketClient &wsc, WiFiClient &wifiClient)
{
  if(!wifiClient.connect(HOST, PORT)) {
    WebSerial.println("Connection failed.");
    return 0;
  }

  WebSerial.println("Connected.");
  wsc.path = wspath;
  wsc.host = HOST;
  if (!wsc.handshake(wifiClient)) {
    WebSerial.println("Handshake failed.");
    return 0;
  }
  WebSerial.println("Handshake successful");
  return 1;
}

void CheckLights(){
  int waittMillis=100;
  int lastM=0;
  int now=millis();
  int LRD;
  int LRDavg;
  for (int i = 0; i < NumOFSwitches ; i++){
    while (now<lastM+waittMillis){
      now=millis();
    }
    LRDavg=0;
    WebSerial.print(i);
    WebSerial.print(": ");
    WebSerial.print(sensor[i].low);
    WebSerial.print(" - ");
    WebSerial.print(sensor[i].high);
    WebSerial.print(" | ");
    LRD=mux.read(sensor[i].pin);
    WebSerial.print(LRD);
    LRDavg+=LRD;
    WebSerial.print("|");
    while (now<lastM+waittMillis+100){
      now=millis();
    }
    LRD=mux.read(sensor[i].pin);
    LRDavg+=LRD;
    WebSerial.print(LRD);
    WebSerial.print("|");
    while (now<lastM+waittMillis+200){
      now=millis();
    }
    LRD=mux.read(sensor[i].pin);
    LRDavg+=LRD;
    WebSerial.print(LRD);
    WebSerial.print("|");
    LRDavg/=3;
    WebSerial.println(LRDavg);
    if (setThresholds){
      sensor[i].low=0.7*LRDavg;
      sensor[i].high=-1.2*LRDavg;
      WebSerial.print(i);
      WebSerial.print(": ");
      WebSerial.print(sensor[i].low);
      WebSerial.print(" - ");
      WebSerial.println(sensor[i].high);
    }
    lastM=millis();
  }
  checkLightBool=false;
  setThresholds=false;
};

int sensorRead(int index){
  int muxChanel = sensor[index].pin;
  int data;
  int data1;
  int data2;
  int data3;
  if (activateSensor){
    data1 = mux.read(muxChanel);
    delay(11);
    data2 = mux.read(muxChanel);
    delay(11);
    data3 = mux.read(muxChanel); 
    data = (data1+data2+data3)/3;
    if (data<sensor[index].high && data>sensor[index].low){
      return 1;
    }
    else{
      return 0;
    }
  }
  else{
    return 1;
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

      // set hub LED color to detected color of sensor and set motor speed dependent on color
      if (CurrentColor == (byte)Color::RED)
      {
        myHub->setLedColor((Color)CurrentColor);
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


class Hubs{
  public:
    Lpf2Hub Hub;
    int currentSpeed;
    byte MotorPort;
    Color trainColor;
    byte colorToStop;
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
int NumberOfHubs = 2;
Hubs myHubs[3];
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




/* Message callback of WebSerial */
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
  else if(d.indexOf(String("action"))>-1){
    SendData(webSocketClientTrain,clientTrain,d);
  }
  else if(d.indexOf(String("motor"))>-1){
    SendData(webSocketClientSwitch,clientSwitch,d);
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
  else if(d.indexOf(String("Reconnect"))>-1){
    if(initWebSocket("/switch",webSocketClientSwitch, clientSwitch) == 0) { return ; };
    if(initWebSocket("/train",webSocketClientTrain, clientTrain) == 0) { return ; };  
  }
  else if (d.indexOf(String("StartScan"))>-1){
    WebSerial.println("Scan Started");
    ScanEnabled = true;
  }
  else if (d.indexOf(String("StopScan"))>-1){
    WebSerial.println("Scan Stoped");
    ScanEnabled = false;
  }
  else if (d.indexOf(String("SendHubs"))>-1){
    WebSerial.println("Hubs Sent");
    sendHubs();
  }
  else if (d.indexOf(String("NumberOfHubs"))>-1){
    NumberOfHubs = d.substring(13,d.length()).toInt();
    WebSerial.print("Setting Number Of Hubs To ");
    WebSerial.println(NumberOfHubs);
  }
  else if (d.indexOf(String("Help"))>-1){
    WebSerial.println("SensorOff");
    WebSerial.println("SensorOn");
    WebSerial.println("StartScan");
    WebSerial.println("StopScan");
    WebSerial.println("SendHubs");
    WebSerial.println("NumberOfHubs:<NUMBER>");
    WebSerial.println("DebugOff");
    WebSerial.println("DebugOn");
    WebSerial.println("CheckLights");
    WebSerial.println("SetThresHolds");
    WebSerial.println("Reconnect");
  }
  WebSerial.println(d);
}





class Switches{
  public:
    int pulse = 0;
    int midPulse;
    bool printed = 0;
    bool allowed = true;
    long lastMillis = 0;
    long counter = 0;
    int sensorPin;
    int servoPin;
    int servoIndex;
    void setParameters(int _servoIndex, int _pulse, String _printed, int _midPulse){
      setPulse(_pulse,(_printed == "Printed" ? true : false));
      midPulse = _midPulse;
      servoIndex =_servoIndex;
      servoPin = servo[_servoIndex];
      sensorPin = sensor[_servoIndex].pin;
    }
    void setPulse(int _pulse, bool _printed){
      if(pulse != _pulse or printed != _printed){
        pulse = _pulse;
        printed = _printed;
        counter++;
      }
    }
    String getSensorStatus(){
      unsigned long currentMillis = millis();
      String ToSend ="";
      int sensorStatus = sensorRead(servoIndex);
      if (sensorStatus == 1){
        if (lastMillis < currentMillis){
          allowed = true;
        }
        if (counter>0){
          if(allowed){
            turnServo();
            ToSend = "{\"motor\":" + String(servoIndex) + ",\"pulse\":" + String(pulse) + ",\"printed\": " + String(printed) + "}";
          }
        }
      }
      else{
        allowed = false;
        lastMillis = currentMillis + interval;
        counter++;
      };
      return ToSend;
    };
    void printValues(){
      if (debug){
        WebSerial.print("motor: ");
        WebSerial.print(servoPin);
        WebSerial.print(" pulse: ");
        WebSerial.print(pulse);
        WebSerial.print(" printed ");
        WebSerial.print(String(printed));
        WebSerial.print(" counter ");
        WebSerial.print(String(counter));      
        WebSerial.print(" lastMillis ");
        WebSerial.println(String(lastMillis));
      }
    }
  private:
    void turnServo(){
      if (debug){
        WebSerial.print("Counter ");
        WebSerial.println(String(counter));
        WebSerial.print("Turn servo ");
        WebSerial.print(String(pulse));
        WebSerial.print(" on ");
        WebSerial.println(String(servoPin));
      }
      pwm.setPWM(servoPin, 0, pulse); 
      if (printed){
        delay(100);
        pwm.setPWM(servoPin, 0, midPulse);
      }
      counter = 0;
    };
};
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
          myHubs[i].setTrainSpeed(jsoninput["speed"].as<int>());
        }
      }
    }
    else if (jsoninput["action"]=="getHubs"){
      sendHubs();
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
  checkLightBool = true;
  setThresholds = true;
/*   myRemote.init(); // initialize the remote hub
  
  for (int i = 0 ; i<NumberOfHubs;i++){
    myHubs[i].Hub.init();    // initialize the listening hub
  } */
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


void setup(void) {
  setupFunc();
}

void loop(void) {
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
