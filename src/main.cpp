#include <WiFi.h>
#include <WebSocketClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();


#define WIFI_SSID "Guber-Kray"
#define WIFI_PASS "Hafnium1985!"

// Server infos
char* HOST = "guberkray.myftp.org";
uint16_t PORT = 80;
uint8_t servo[9] = {0,1,2,3,4,5,6,7};
uint8_t sensor[9] = {GPIO_NUM_13,GPIO_NUM_12,GPIO_NUM_14,GPIO_NUM_27,GPIO_NUM_15,GPIO_NUM_19,GPIO_NUM_18,GPIO_NUM_4};
const long interval = 200;  
bool activateSensor = true;
bool debug = false;

int sensorRead(uint8_t sensorPin){
  if (activateSensor){
    return 1-digitalRead(sensorPin);
  }
  else{
    return 1;
  }
}


WebSocketClient webSocketClientSwitch;
WebSocketClient webSocketClientTrain;
WiFiClient clientSwitch;
WiFiClient clientTrain;

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
      sensorPin = sensor[_servoIndex];
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
        Serial.print("motor: ");
        Serial.print(servoPin);
        Serial.print(" pulse: ");
        Serial.print(pulse);
        Serial.print(" printed ");
        Serial.print(String(printed));
        Serial.print(" counter ");
        Serial.print(String(counter));      
        Serial.print(" lastMillis ");
        Serial.println(String(lastMillis));
      }
    }
  private:
    void turnServo(){
      if (debug){
        Serial.print("Counter ");
        Serial.println(String(counter));
        Serial.print("Turn servo ");
        Serial.print(String(pulse));
        Serial.print(" on ");
        Serial.println(String(servoPin));
      }
      pwm.setPWM(servoPin, 0, pulse); 
      if (printed){
        delay(100);
        pwm.setPWM(servoPin, 0, midPulse);
      }
      counter = 0;
    };
};

int NumOFSwitches=0;
Switches switches[15];



String dataToSend; // update this with the value you wish to send to the server


void onDataReceived(String &data)
{
  DynamicJsonDocument jsoninput(2048);
  DeserializationError error = deserializeJson(jsoninput, data);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
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
  else if (jsoninput.containsKey("SensorConfig")){
    Serial.println("SensorConfig");
    JsonArray SensorConfig = jsoninput["SensorConfig"].as<JsonArray>();
    Serial.println(SensorConfig);
    int i = 0;
    for (JsonVariant senconf : SensorConfig) {
      sensor[i]=senconf.as<uint8_t>();
      i++;
      if(debug){
        Serial.print("Sensor ");
        Serial.print(String(i));
        Serial.print(" Pin ");
        Serial.println(String(sensor[i]));
      }
    }
  }
}

char initWebSocket(char* wspath,WebSocketClient &wsc,WiFiClient &client)
{
  if(!client.connect(HOST, PORT)) {
    Serial.println("Connection failed.");
    return 0;
  }

  Serial.println("Connected.");
  wsc.path = wspath;
  wsc.host = HOST;
  if (!wsc.handshake(client)) {
    Serial.println("Handshake failed.");
    return 0;
  }
  Serial.println("Handshake successful");
  return 1;
}


char connect()
{
  WiFi.begin(WIFI_SSID, WIFI_PASS); 

  Serial.println("Waiting for wifi");
  int timeout_s = 30;
  while (WiFi.status() != WL_CONNECTED && timeout_s-- > 0) {
      delay(1000);
      Serial.print(".");
  }
  
  if(WiFi.status() != WL_CONNECTED)
  {
    Serial.println("unable to connect, check your credentials");
    return 0;
  }
  else
  {
    Serial.println("Connected to the WiFi network");
    Serial.println(WiFi.localIP());
    return 1;
  }
}

void SendData(WebSocketClient &wsc, WiFiClient &c ,String &dataToSend){
  if (c.connected()) {
    if(dataToSend.length() > 0)
    {
      wsc.sendData(dataToSend);
      if (debug){
        Serial.print("Data Sent: ");
        Serial.println(dataToSend);
      };
    }
    dataToSend = "";
  } else {
    Serial.println("Client disconnected. (send)");
    c.connect(HOST, PORT);
  }
};
void GetData(WebSocketClient &wsc, WiFiClient &c){
  String data;
 
  if (c.connected()) {
    wsc.getData(data);
    if (data.length() > 0) {
      onDataReceived(data);
    }
  } else {
    Serial.println("Client disconnected. (get)");
    c.connect(HOST, PORT);
  }
};

void setupFunc()
{
  Serial.begin(115200);
  // try to connect to the wifi
  Serial.println("Wifi:");
  if(connect() == 0) { return ; };
  // once connected to the wifi, let's reach our server
  if(initWebSocket("/ws/switch",webSocketClientSwitch,clientSwitch) == 0) { return ; };
  if(initWebSocket("/ws/train",webSocketClientTrain,clientTrain) == 0) { return ; };
  dataToSend = "{\"action\":\"getConfigESP\"}";
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50); 
}

void setup()
{
  setupFunc();
}



void loop() {
  if (Serial.available()) {
    dataToSend=Serial.readString();
  }
  if (dataToSend.indexOf(String("SensorOff"))>-1){
    Serial.println("Sensors Deactivated");
    activateSensor = false;
    dataToSend = "";
  }
  else if (dataToSend.indexOf(String("SensorOn"))>-1){
    Serial.println("Sensors Activated");
    activateSensor = true;
    dataToSend = "";
  }
  else if (dataToSend.indexOf(String("DebugOff"))>-1){
    Serial.println("Debug Deactivated");
    debug = false;
    dataToSend = "";
  }
  else if (dataToSend.indexOf(String("DebugOn"))>-1){
    Serial.println("Debug Activated");
    debug = true;
    dataToSend = "";
  }
  else if(dataToSend.indexOf(String("action"))>-1){
    SendData(webSocketClientTrain,clientTrain,dataToSend);
  }
  else if(dataToSend.indexOf(String("motor"))>-1){
    SendData(webSocketClientSwitch,clientSwitch,dataToSend);
  }
  else if(dataToSend.indexOf(String("Restart"))>-1){
    setupFunc();
  }

  GetData(webSocketClientSwitch,clientSwitch);
  GetData(webSocketClientTrain,clientTrain);
  
  for (int i = 0; i < NumOFSwitches ; i++){
    dataToSend = switches[i].getSensorStatus();
    if(dataToSend.length() > 0){
      SendData(webSocketClientSwitch,clientSwitch,dataToSend);
    }
  }
}
