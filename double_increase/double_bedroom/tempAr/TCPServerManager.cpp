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

void acceptTCPConnections(void (*onMessage)(WiFiClient &client, String message)) {
  while (WiFiClient newClient = server.available()) {
    // 如果有新连接并且TCP通道连接着，则可能需要替换新连接
    if (newClient.connected()) {
      IPAddress ip = newClient.remoteIP();
      String remoteId = ipToString(ip);
      
      for (auto &p : peers) {
        if (ipToString(p.ip) == remoteId) {
          // 如果旧连接还活着，就不替换
          if (p.client && p.client.connected()) {
            Serial.println("已有连接存在，不替换: " + p.id);
            p.lastActive = millis();
            newClient.stop(); // 拒绝重复连接
          } 
          else {
            p.client = newClient;
            p.lastActive = millis();
            Serial.println("接收新连接来自: " + p.id);
          }
          break;
        }
      }

      // 如果有数据，就处理
      if (newClient.available()) {
        String msg = newClient.readStringUntil('\n');
        onMessage(newClient, msg);
      }
    }
  }
}
