#include "HardwareSerial.h"
#include "ConnectionManager.h"
#include "config.h"
#include <Arduino.h>
#include <vector>

void addOrUpdatePeer(String id, IPAddress ip) {
  for (auto &p : peers) {
    if (p.id == id) {
      p.ip = ip;
      return;
    }
  }
  Peer newPeer = { id, ip };
  peers.push_back(newPeer);
}

bool sendToPeer(String id, const String &msg) {
  for (auto &p : peers) {
    if (p.id == id)
    {
      if (!p.client.connected()) {
        Serial.println("Connecting to " + id);
        if (!p.client.connect(p.ip, TCP_PORT)) {
          Serial.println("Connection failed to " + id);
          return false;
        }
      }
      p.client.println(msg);
      p.lastActive = millis();
      Serial.println("Sent to " + id + ": " + msg);
      return true;
    }
  }
  return false;
}

void cleanConnections() {
  for (auto &p : peers) {
    if (millis() - p.lastActive > TIMEOUT_MS || !p.client.connected()) {
      p.client.stop();
    }
  }
}