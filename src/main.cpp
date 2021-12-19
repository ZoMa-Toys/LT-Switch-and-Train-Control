#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <WebSocketClient.h>
#include <WebSerial.h>
#include <ArduinoJson.h>
#include <Adafruit_PWMServoDriver.h>
#include "Mux.h"

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
uint16_t PORT = 80;
uint8_t servo[8] = {0,1,2,3,4,5,6,7};
sensors sensor[8] = {{0,500,2500},{1,500,2500},{2,500,2500},{3,500,2500},{7,500,2500},{5,500,2500},{6,500,2500},{4,500,2500}};
/* int sensor[8] = {0,1,2,3,7,5,6,4}; */

int analogThresholdHigh = 2500;
int analogThresholdLow = 500;
const long interval = 700;  
bool activateSensor = true;
bool debug = false;
bool checkLightBool = false;
bool setThresholds = false;
int NumOFSwitches=0;
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



char initWebSocket(char* wspath,WebSocketClient &wsc, WiFiClient &wifiClient)
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

int sensorRead(int muxChanel){
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
    if (data<analogThresholdHigh && data>analogThresholdLow){
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
  else if (d.indexOf(String("analogThresholdHigh"))>-1){
    analogThresholdHigh = d.substring(20,d.length()).toInt();
    WebSerial.print("analogThresholdHigh set to ");
    WebSerial.println(analogThresholdHigh);
  }
  else if (d.indexOf(String("analogThresholdLow"))>-1){
    analogThresholdLow = d.substring(19,d.length()).toInt();
    WebSerial.print("analogThresholdLow set to ");
    WebSerial.println(analogThresholdLow);
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
      int sensorStatus = sensorRead(sensorPin);
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
  /* else if (jsoninput.containsKey("SensorConfig")){
    JsonArray SensorConfig = jsoninput["SensorConfig"].as<JsonArray>();
    if (debug){
      WebSerial.println(SensorConfig);
    }
    int i = 0;
    for (JsonVariant senconf : SensorConfig) {
      sensor[i]=senconf.as<uint8_t>();
      i++;
      if(debug){
        WebSerial.print("Sensor ");
        WebSerial.print(String(i));
        WebSerial.print(" Pin ");
        WebSerial.println(String(sensor[i]));
      }
    }
  } */
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
  dataToSend = "{\"action\":\"getConfigESP\"}";
  SendData(webSocketClientTrain,clientTrain,dataToSend);
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
    CheckLights();
  }
}
