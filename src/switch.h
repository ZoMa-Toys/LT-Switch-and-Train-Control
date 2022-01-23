#ifndef switch_h
#define switch_h
#include "WiFIAndWeb.h"
#include <Adafruit_PWMServoDriver.h>
#include "Mux.h"

using namespace admux;

typedef struct {
    int pin;
    int low;
    int high;
} sensors; 

Mux mux(Pin(35, INPUT, PinType::Analog), Pinset(27, 26, 25)); 
uint8_t servo[8] = {0,1,2,3,4,5,6,7};
sensors sensor[8] = {{0,500,2500},{1,500,2500},{2,500,2500},{3,500,2500},{7,500,2500},{5,500,2500},{6,500,2500},{4,500,2500}};
const long interval = 500; 
int NumOFSwitches=0;
bool checkLightBool = true;
bool setThresholds = true;
bool activateSensor = true;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

void setupPWM(){
    pwm.begin();
    pwm.setOscillatorFrequency(27000000);
    pwm.setPWMFreq(50);
}




void CheckLights(){
  debugPrint("CheckLights:" + String(checkLightBool) + "setThresholds: " + String(setThresholds));
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
    LRD=mux.read(sensor[i].pin);
    LRDavg+=LRD;
    WebSerial.print(String(i) + ": " + String(sensor[i].low) + " - " + String(sensor[i].high) + " || " + String(LRD) + "|" );
    while (now<lastM+waittMillis+100){
      now=millis();
    }
    LRD=mux.read(sensor[i].pin);
    LRDavg+=LRD;
    WebSerial.print(String(LRD) + "|" );
    while (now<lastM+waittMillis+200){
      now=millis();
    }
    LRD=mux.read(sensor[i].pin);
    LRDavg+=LRD;
    LRDavg/=3;
    WebSerial.println(String(LRD) + "||" + String(LRDavg));
    if (setThresholds){
      sensor[i].low=0.5*LRDavg;
      sensor[i].high=LRDavg+(4095-LRDavg)/2;
      WebSerial.println(String(i) + ": " + String(sensor[i].low) + " - " + String(sensor[i].high));
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
    bool shouldSendState=true;
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
      debugPrint("motor: " + String(servoPin) + " pulse: " + String(pulse) + " printed " + String(printed) + " counter " + String(counter) + " lastMillis " +  String(lastMillis));
    }
  private:
    void turnServo(){
      debugPrint("Counter " + String(counter) + " Turn servo " + String(pulse) + " on " + String(servoPin) );
      
      pwm.setPWM(servoPin, 0, pulse); 
      if (printed){
        delay(100);
        pwm.setPWM(servoPin, 0, midPulse);
      }
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
        int midPulse = (swconf["pulse"]["Straight"].as<int>() + swconf["pulse"]["Turn"].as<int>())/2;
        switches[i].setParameters(i,swconf["pulse"][swconf["switched"].as<String>()].as<int>(),swconf["printed"].as<String>(),midPulse);
        switches[i].printValues();
        i++;
    }
    NumOFSwitches = i;
    debugPrint(String(NumOFSwitches) + " switches configured");
}

#endif 