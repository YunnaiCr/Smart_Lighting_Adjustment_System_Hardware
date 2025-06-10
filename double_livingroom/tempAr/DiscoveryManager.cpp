#include <Arduino.h>
#include "DiscoveryManager.h"
#include <WiFiUdp.h>
#include "config.h"
WiFiUDP udp;

void initDiscovery() {
  udp.begin(DISCOVERY_PORT);
}

void sendDiscoveryPacket() {
  const char* msgPrefix = "DISCOVER:";
  udp.beginPacket("255.255.255.255", DISCOVERY_PORT);
  udp.write((const uint8_t*)msgPrefix, strlen(msgPrefix));
  udp.write((const uint8_t*)DEVICE_ID, strlen(DEVICE_ID));
  udp.endPacket();
}

void handleDiscoveryPacket(void (*onDiscovered)(String id, IPAddress ip)) {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char buf[65]; // 1 extra for null terminator
    int len = udp.read((uint8_t*)buf, packetSize);
    if (len > 0) {
      buf[len] = '\0'; // 确保字符串结尾
      String msg = String(buf);
      if (msg.startsWith("DISCOVER:")) {
        String otherID = msg.substring(9);
        if (otherID != DEVICE_ID) {
          const char* responsePrefix = "RESPONSE:";
          udp.beginPacket(udp.remoteIP(), DISCOVERY_PORT);
          udp.write((const uint8_t*)responsePrefix, strlen(responsePrefix));
          udp.write((const uint8_t*)DEVICE_ID, strlen(DEVICE_ID));
          udp.endPacket();
          if (onDiscovered) onDiscovered(otherID, udp.remoteIP());
        }
      }
      else if (msg.startsWith("RESPONSE:")) {
        String otherID = msg.substring(9);
        if (onDiscovered) onDiscovered(otherID, udp.remoteIP());
      }
    }
  }
}