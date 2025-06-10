#pragma once
#include "config.h"
#include "network.h"
#include <Arduino.h>

void startTCPServer();
void acceptTCPConnections(void (*onMessage)(WiFiClient &client, String message));