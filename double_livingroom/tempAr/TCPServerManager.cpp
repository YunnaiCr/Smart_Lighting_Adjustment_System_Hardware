#include "WiFiClient.h"
#include "core_esp8266_features.h"
#include <ESP8266WiFi.h>
#include "config.h"
#include "network.h"
#include <Arduino.h>
#include "TCPServerManager.h"
#include "ConnectionManager.h"

WiFiServer server(TCP_PORT);

void startTCPServer() {
  server.begin();
}

void acceptTCPConnections() {
  WiFiClient newClient = server.available();
  if (!newClient) return;

  Serial.print("新连接尝试，连接状态: ");
  Serial.println(newClient.connected() ? "已连接" : "未连接");

  IPAddress ip = newClient.remoteIP();
  String remoteId = ipToString(ip);
  Serial.print("客户端IP: ");
  Serial.println(remoteId);

  for (auto &p : peers) {
    if (p.id == remoteId) {
      if (p.client.connected()) {
        Serial.println("替换旧连接: " + p.id);
        p.client.stop();
      }
      p.client = newClient;
      p.lastActive = millis();
      Serial.println("新连接来自: " + p.id);
      return;
    }
  }

  Serial.println("新连接来自未知Peer，自动添加: " + remoteId);
  peers.push_back({remoteId, ip, newClient, millis()});
}

void readMessagesFromPeers(void (*onMessage)(WiFiClient &client, String message)) {
  for (auto &p : peers) {
    if (p.client && p.client.connected()) {
      while (p.client.available()) {
        String msg = p.client.readStringUntil('\n');
        Serial.print("收到来自 ");
        Serial.print(p.id);
        Serial.print(" 的消息: ");
        Serial.println(msg);
        onMessage(p.client, msg);
        p.lastActive = millis();
      }
    }
  }
}

