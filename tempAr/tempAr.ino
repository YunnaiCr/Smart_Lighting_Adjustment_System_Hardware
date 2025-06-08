// main.ino
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h> 
#include "JsonFactory.h" 

// 引入自定义头文件
#include "config.h"
#include "state.h"
#include "network.h"
#include "light_control.h"
#include "mqtt_handler.h"

// 定义全局状态变量的实际存储
String operationMode = "manualMode";
String voiceCommand = "None";
int part = 0;
String sceneMode = "Default"; // 初始情景模式
int voiceReddata = 0;
int voiceGreendata = 0;
int voiceBluedata = 0;
int manualRedValue = DEFAULT_SCENE_R; // 与初始sceneMode匹配
int manualGreenValue = DEFAULT_SCENE_G;
int manualBlueValue = DEFAULT_SCENE_B;
unsigned long lastStatusUpdate = 0;
unsigned long lastReconnectAttempt = 0;
int currentColorBaseR = WHITE_MODE_R; // 默认白色
int currentColorBaseG = WHITE_MODE_G;
int currentColorBaseB = WHITE_MODE_B;
String currentBaseColorName = "white";
float currentBrightnessLevel = 1.0f;
bool manualModeEverEntered = false; // 初始为 false
void setup() {
  Serial.begin(9600);
  Serial.println("\nESP8266智能灯光系统启动...");

  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  setLEDColor(255, 255, 255); // 初始关闭灯光

  connectToWiFi();
  syncTime();

  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);

  // 第一次进入手动模式的逻辑放在 setOperationMode 中处理
  setOperationMode("manualMode"); // 这会初始化 part 和 sceneMode 并设置灯光
  manualModeEverEntered = true; // 确保下次不会重复默认初始化

  connectToMQTTBroker(); //
  Serial.println("系统初始化完成");
}

void loop() {
  if (!mqtt_client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      Serial.println("MQTT断开连接，尝试重连...");
      if (connectToMQTTBroker()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    mqtt_client.loop();
  }

  if (operationMode == "autoMode") {
    adjustLightingBasedOnSensor();
  }
  else if (operationMode == "voiceMode") {
    checkVoiceCommands();
  }

  delay(50);
  yield();
}
