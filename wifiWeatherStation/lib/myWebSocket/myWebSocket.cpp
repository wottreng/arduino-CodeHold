#include <Arduino.h>
#include <WebSocketsClient.h>

// custom libs
#include <myTemp.h>
#include "myWebSocket.h"


// local variables ----------------------
// websocket server host
WebSocketsClient webSocket;
String serverName = "10.42.0.95";
int port = 5000;
String path = "/";

// socket status
String websocketStatus = "";
bool activate_webSocket = false;

// functions ===========================================

// check websocket activity
void websocketLoop(){
  if(activate_webSocket) webSocket.loop();
}

String getSocketStatus(){
    return websocketStatus;
}

// socket events and handlers
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    //Serial.println("[WSc] Disconnected!\n");
    websocketStatus = "[WSc] Disconnected!";
    break;

  case WStype_CONNECTED:
    //Serial.printf("[WSc] Connected to url: %s\n", payload);
    websocketStatus = "[WSc] Connected to url: "  ;
    websocketStatus += (char*)payload;
    // send message to server when Connected
    // webSocket.sendTXT("Connected");
    break;

  case WStype_TEXT:
    //Serial.printf("[WSc] got text: %s\n", payload);
      websocketStatus = "[WSc] got text: ";
      websocketStatus += (char*)payload ;
      websocketStatus = "[WSc] got text: "; //+ (char *)payload;
      if (strcmp((char*)payload, "dataReq") == 0) {
          String data = returnTempString();
          webSocket.sendTXT(data);
          // Serial.println("sent data");
      }
    delay(1000); // debounce
    break;

  case WStype_BIN:
    //Serial.printf("[WSc] get binary length: %u\n", length);
    //hexdump(payload, length);
    // send data to server
    // webSocket.sendBIN(payload, length);
    break;

  case WStype_PING:
    // pong will be send automatically
    //Serial.println("[WSc] get ping\n");
    break;

  case WStype_PONG:
    // answer to a ping we send
    //Serial.println("[WSc] got pong\n");
    break;

  case WStype_ERROR:
      websocketStatus = "ERROR";
      break;
  }
}

// websocket switch to turn on/off
void webSocketServerSwitch(bool status)
{
    if (status) {
        activate_webSocket = true;
        // server address, port and URL
        webSocket.begin(serverName, port, path);
        // event handler
        webSocket.onEvent(webSocketEvent);
        // use HTTP Basic Authorization this is optional remove if not needed
        // **webSocket.setAuthorization("user", "Password");
        // try ever 30 sec again if connection has failed
        webSocket.setReconnectInterval(30000);
        // start heartbeat (optional)
        // ping server every 60s
        // expect pong from server within 3000 ms
        // consider connection disconnected if pong is not received 2 times
        webSocket.enableHeartbeat(60000, 3000, 2);
    } else {
        activate_webSocket = false;
        webSocket.disconnect();
    }
}