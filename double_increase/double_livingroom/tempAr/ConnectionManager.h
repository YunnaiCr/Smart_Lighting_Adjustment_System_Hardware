#pragma once
#include "config.h"
#include <Arduino.h>

void addOrUpdatePeer(String id, IPAddress ip);
bool sendToPeer(String id, const String &msg);
void cleanConnections();