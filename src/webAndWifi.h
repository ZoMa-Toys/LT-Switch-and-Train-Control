#ifndef webAndWifi_h
#define webAndWifi_h

#include "paramteres.h"

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <WebSocketClient.h>
#include <ArduinoJson.h>

String dataToSend; // update this with the value you wish to send to the server
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





#endif