#include "IPAddress.h"
// network.h
#ifndef NETWORK_H
#define NETWORK_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <BearSSLHelpers.h> // for BearSSL::WiFiClientSecure
#include "config.h" // 包含配置信息
#include "state.h"  // 包含状态变量

// 声明外部链接的MQTT客户端
extern BearSSL::WiFiClientSecure espClient;
extern PubSubClient mqtt_client;

void connectToWiFi();
void syncTime();
bool connectToMQTTBroker();
String ipToString(IPAddress ip);

#endif
