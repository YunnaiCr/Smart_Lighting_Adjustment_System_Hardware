// mqtt_handler.h
#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <ArduinoJson.h> // for StaticJsonDocument
#include <PubSubClient.h> // for PubSubClient
#include "JsonFactory.h"  // for ConverseDataToJson (假设这个是你的JsonFactory.h)
#include "state.h"        // 包含状态变量
#include "config.h"       // 包含配置信息 (如mqtt_status_topic)
#include "light_control.h" // 包含灯光控制函数 (如setOperationMode, setSceneMode等)
#include "network.h"       // 包含外部链接的mqtt_client

void mqttCallback(char* topic, byte* payload, unsigned int length);
void parseStatusString(const String& statusStr);
void sendStatusString();
void sendModeChangeToApp(const String& newMode);
void switchFromVoiceMode(const String& newMode);
#endif
