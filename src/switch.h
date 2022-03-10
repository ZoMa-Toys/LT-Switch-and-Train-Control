#ifndef switch_h
#define switch_h
#include "WifiAndWeb.h"

#ifndef MINSWITCHID
#define MINSWITCHID  0
#endif

#ifndef MAXSWITCHID 
#define MAXSWITCHID  7
#endif

#if defined (ESP32)
  #include <Adafruit_PWMServoDriver.h>
  #include "Mux.h"
  using namespace admux;
  Mux mux(Pin(35, INPUT, PinType::Analog), Pinset(27, 26, 25)); 
  Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
  void setupPWM_Servo(){
    pwm.begin();
    pwm.setOscillatorFrequency(27000000);
    pwm.setPWMFreq(50);
  } 
  void turnServoGeneral(int pulse, bool printed, int servoPin,int midPulse, int turnPulse){
    pwm.setPWM(servoPin, 0, pulse); 
      if (printed){
        delay(100);
        pwm.setPWM(servoPin, 0, midPulse);
      }
  }
#else
  #include "Servo.h"
  typedef struct {
    int RED;
    uint8_t R_LOW_HIGH;
    int GREEN;
    uint8_t G_LOW_HIGH;
    int BLUE;
    uint8_t B_LOW_HIGH;
  } ledtype;

  ledtype LEDS[3] = {{D0,LOW,D5,HIGH,D6,LOW},{D2,LOW,D3,HIGH,D4,LOW},{D7,LOW,D8,HIGH,RX,LOW}};
  void LEDSetup(){
    for (auto led: LEDS){
      pinMode(led.RED,OUTPUT);
      digitalWrite(led.RED,led.R_LOW_HIGH);
      pinMode(led.GREEN,OUTPUT);
      digitalWrite(led.GREEN,led.G_LOW_HIGH);
      pinMode(led.BLUE,OUTPUT);
      digitalWrite(led.BLUE,led.B_LOW_HIGH);
    }
  }

  void switchLEDSide(bool isTurn){
    if (isTurn){
      LEDS[0].B_LOW_HIGH=HIGH;
      LEDS[1].B_LOW_HIGH=LOW;
    }
    else{
      LEDS[0].B_LOW_HIGH=LOW;
      LEDS[1].B_LOW_HIGH=HIGH;
    }
    digitalWrite(LEDS[0].BLUE,LEDS[0].B_LOW_HIGH);
    digitalWrite(LEDS[1].BLUE,LEDS[1].B_LOW_HIGH);
  }

  void switchLED_RED_GREEN(int side,bool greenbool){
    ledtype L = LEDS[side];
    debugPrint("LEDS:"+String(side)+" green: "+String(greenbool));
    if (greenbool){
      L.R_LOW_HIGH=LOW;
      L.G_LOW_HIGH=HIGH;
    }
    else{
      L.R_LOW_HIGH=HIGH;
      L.G_LOW_HIGH=LOW;
    }
    debugPrint("RED PIN:"+String(L.RED)+" low_high: "+String(L.R_LOW_HIGH));
    debugPrint("Green PIN:"+String(L.GREEN)+" low_high: "+String(L.G_LOW_HIGH));
    digitalWrite(L.RED,L.R_LOW_HIGH);
    digitalWrite(L.GREEN,L.G_LOW_HIGH);
  }

  Servo myservo;
  int pulseToangle(int pulse){
    return map(pulse,150,600,0,180);
  }
  void setupPWM_Servo(){
    myservo.attach(D1);
  }
  void turnServoGeneral(int pulse, bool printed, int servoPin,int midPulse, int turnPulse){
    if(pulse==turnPulse){
      switchLEDSide(true);
    }
    else{
      switchLEDSide(false);
    }
    myservo.write(pulseToangle(pulse));
    if (printed){
      delay(100);
      myservo.write(pulseToangle(midPulse));
    }
  }

#endif




typedef struct {
    int pin;
    int low;
    int high;
} sensors; 

int minswitchID = MINSWITCHID;
int maxswitchID = MAXSWITCHID;
uint8_t servo[10] = {0,1,2,3,4,5,6,7,D1,D1};
sensors sensor[10] = {{0,500,2500},{1,500,2500},{2,500,2500},{3,500,2500},{7,500,2500},{5,500,2500},{6,500,2500},{4,500,2500},{A0,500,2500},{A0,500,2500}};
const long interval = 500; 
int NumOFSwitches=0;
bool checkLightBool = false;
bool setThresholds = false;
bool activateSensor = true;


int readSensor(int pin){
  #if defined (ESP32)
    return mux.read(pin);
  #else
    return analogRead(pin);
  #endif
}


void CheckLights(){
  debugPrint("CheckLights:" + String(checkLightBool) + "setThresholds: " + String(setThresholds));
  #if defined (ESP32)
    int maxvalue=4095;
  #else
    int maxvalue=1023;
  #endif

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
    LRD=readSensor(sensor[i].pin);
    LRDavg+=LRD;
    debugPrint(String(i) + ": " + String(sensor[i].low) + " - " + String(sensor[i].high) + " || " + String(LRD) + "|" );
    while (now<lastM+waittMillis+100){
      now=millis();
    }
    LRD=readSensor(sensor[i].pin);
    LRDavg+=LRD;
    debugPrint(String(LRD) + "|" );
    while (now<lastM+waittMillis+200){
      now=millis();
    }
    LRD=readSensor(sensor[i].pin);
    LRDavg+=LRD;
    LRDavg/=3;
    debugPrint(String(LRD) + "||" + String(LRDavg));
    if (setThresholds){
      sensor[i].low=0.5*LRDavg;
      sensor[i].high=LRDavg+(maxvalue-LRDavg)/2;
      debugPrint(String(i) + ": " + String(sensor[i].low) + " - " + String(sensor[i].high));
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
    data1 = readSensor(muxChanel);
    delay(11);
    data2 = readSensor(muxChanel);
    delay(11);
    data3 = readSensor(muxChanel); 
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


class Switches{
  public:
    int pulse = 0;
    int midPulse;
    int turnPulse;
    int straightPulse;
    bool printed = 0;
    bool allowed = true;
    long lastMillis = 0;
    long counter = 0;
    int sensorPin;
    int servoPin;
    int servoIndex;
    bool shouldSendState=true;
    void setParameters(int _servoIndex, int _pulse, String _printed, int _straightPulse, int _turnPulse){
      setPulse(_pulse,(_printed == "Printed" ? true : false));
      midPulse = (_straightPulse + _turnPulse)/2;
      turnPulse = _turnPulse;
      straightPulse = _straightPulse;
      servoIndex =_servoIndex;
      servoPin = servo[_servoIndex];
      sensorPin = sensor[_servoIndex].pin;
    }
    void setPulse(int _pulse, bool _printed){
      if(pulse != _pulse or printed != _printed){
        pulse = _pulse;
        printed = _printed;
        counter++;
        shouldSendState=true;
      }
    }
    void getSensorStatus(){
      unsigned long currentMillis = millis();
      int sensorStatus = sensorRead(servoIndex);
      if (sensorStatus == 1){
        if (lastMillis < currentMillis){
          allowed = true;
        }
        if (counter>0){
          if(allowed){
            turnServo();
            if(shouldSendState){
              messageJSONToSend["motor"]=servoIndex;
              messageJSONToSend["pulse"]=pulse;
              messageJSONToSend["printed"]=printed;
              shouldSendState=false;
            }

          }
        }
      }
      else{
        allowed = false;
        lastMillis = currentMillis + interval;
        counter++;
      };
    };
    void printValues(){
      debugPrint("motor: " + String(servoPin) + "(" + String(servoIndex) + ") pulse: " + String(pulse) + " printed " + String(printed) + " counter " + String(counter) + " lastMillis " +  String(lastMillis));
    }
  private:
    void turnServo(){
      debugPrint("Counter " + String(counter) + " Turn servo " + String(pulse) + " on " + String(servoPin) );
      
      turnServoGeneral(pulse,printed,servoPin,midPulse,turnPulse);
      
      counter = 0;
    };
};

Switches switches[15];

void SwitchTrack(StaticJsonDocument<2048> messageJSON){
    switches[messageJSON["motor"].as<int>()].printValues();
    switches[messageJSON["motor"].as<int>()].setPulse(messageJSON["pulse"].as<int>(),messageJSON["printed"].as<bool>());
    switches[messageJSON["motor"].as<int>()].printValues();
}

void SetSwitches(StaticJsonDocument<2048> messageJSON){
    int i = 0;
    for (JsonVariant swconf : messageJSON.as<JsonArray>()) {
        int straightPulse = swconf["pulse"]["Straight"].as<int>();
        int tunrPulse = swconf["pulse"]["Turn"].as<int>();
        switches[i].setParameters(i,swconf["pulse"][swconf["switched"].as<String>()].as<int>(),swconf["printed"].as<String>(),straightPulse,tunrPulse);
        switches[i].printValues();
        i++;
    }
    NumOFSwitches = i;
    debugPrint(String(NumOFSwitches) + " switches configured");
    setThresholds = true;
    activateSensor = true;
}

#endif 
