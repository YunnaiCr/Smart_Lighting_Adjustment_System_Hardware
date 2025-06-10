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
#include "DiscoveryManager.h"
#include "TCPServerManager.h"
#include "ConnectionManager.h"


//派对模式之渐变
unsigned long lastPartyUpdate = 0;
int partyHue = 0; // 0-360度的HSV色相值
bool partyModeActive = false;
//派对模式之频闪
unsigned long lastStrobeUpdate = 0;
bool strobeState = false;
int strobeSpeed = 100; // 闪烁间隔(毫秒)
int strobeColor = 1;   // 0=白光，1=彩色，2=渐变色
int strobeColorHue = 0; // 用于彩色模式的色相值
// 定义全局状态变量的实际存储
String operationMode = "manualMode";
String voiceCommand = "None";
bool part = false;
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

  // 寻找同局域网ESP板
  initDiscovery();
  startTCPServer();
  sendDiscoveryPacket();

  // 第一次进入手动模式的逻辑放在 setOperationMode 中处理
  setOperationMode("manualMode"); // 这会初始化 part 和 sceneMode 并设置灯光
  manualModeEverEntered = true; // 确保下次不会重复默认初始化

  connectToMQTTBroker(); //
  Serial.println("系统初始化完成");
}

void onDiscovered(String id, IPAddress ip) {
  Serial.printf("Discovered: %s at %s\n", id.c_str(), ip.toString().c_str());
  addOrUpdatePeer(id, ip);
}
void onMessage(WiFiClient &client, String msg) {
  Serial.printf("收到消息: %s\n", msg.c_str());
  
  handleRemoteCommand(msg);
}

void loop() {
  handleDiscoveryPacket(onDiscovered);
  acceptTCPConnections();  // 只处理新连接
  readMessagesFromPeers(onMessage);  // 处理所有客户端消息
  cleanConnections();  // 清理超时连接

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
  if (operationMode == "manualMode" && part == false && sceneMode == "Party" && partyModeActive) {
    updatePartyLights();
  }
  if (operationMode == "autoMode") {
    adjustLightingBasedOnSensor();
  }
  else if (operationMode == "voiceMode") {
    checkVoiceCommands();
  }

  // 发送板间消息，这个只是测试例
  static unsigned long last = 0;
  if (millis() - last >1000) {
    last = millis();
    // 板名 + 要发的信息，建议放在专门的文件里，比如voice的文件
    // 板名记得改，在config.h里
    // 函数返回一个bool值，表示发送成功与否
    sendToPeer(REMOTE_ESP_ID, "ping");
  }

  delay(50);
  yield();
}
