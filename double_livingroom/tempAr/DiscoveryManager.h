#pragma once
#include "config.h"
#include <Arduino.h>

void initDiscovery();
void sendDiscoveryPacket();
void handleDiscoveryPacket(void (*onDiscovered)(String id, IPAddress ip));