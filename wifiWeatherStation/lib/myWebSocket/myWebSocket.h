#pragma once

#include <Arduino.h>
#include <WebSocketsClient.h>



void websocketLoop();

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);

void webSocketServerSwitch(bool status);

String getSocketStatus();