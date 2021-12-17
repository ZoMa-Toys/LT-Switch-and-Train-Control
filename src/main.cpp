#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <WebSocketClient.h>
#include <WebSerial.h>
#include <ArduinoJson.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

const char* ssid = "Guber-Kray";
const char* password = "Hafnium1985!";
char* HOST = "guberkray.myftp.org";
uint16_t PORT = 80;
uint8_t servo[9] = {0,1,2,3,4,5,6,7};
uint8_t sensor[9] = {36,39,34,35,32,33,13,4};
const int analogThreshold = 3000;
const long interval = 200;  
bool activateSensor = false;
bool debug = false;


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
/*   else if(d.indexOf(String("Restart"))>-1){
    setupFunc();
  } */


  WebSerial.println(d);
}


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

int sensorRead(uint8_t sensorPin){
  if (activateSensor){
    if (sensorPin<30){
      return 1-digitalRead(sensorPin);
    }
    else{
      int sv = analogRead(sensorPin);
      if (debug){
        WebSerial.println(sv);
      }
      if(sv>analogThreshold){
        return 0;
      }
      else{
        return 1;
      }
    }
  }
  else{
    return 1;
  }
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

int NumOFSwitches=0;
Switches switches[15];

String dataToSend; // update this with the value you wish to send to the server


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
  else if (jsoninput.containsKey("SensorConfig")){
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
    switches[i].printValues();
    if(dataToSend.length() > 0){
      SendData(webSocketClientSwitch,clientSwitch,dataToSend);
    }
  }
}
