#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include "JsonFactory.h" 

// WiFi设置
const char *ssid = "Roxy111";
const char *password = "czy54012";

// MQTT设置
const char *mqtt_broker = "jfe2a84f.ala.cn-hangzhou.emqxsl.cn";
const char *mqtt_topic = "light/livingroom";
const char *mqtt_status_topic = "light/livingroom/status";
const char *mqtt_username = "Yunnai";
const char *mqtt_password = "azathoth";
const int mqtt_port = 8883;

// NTP服务器设置
const char *ntp_server = "pool.ntp.org";
const long gmt_offset_sec = 28800;
const int daylight_offset_sec = 0;

// 引脚定义 - ESP8266 NodeMCU引脚
const int photoSensorPin = A0;  // 光敏传感器
const int redPin = 5;    // D1(GPIO5) - 红色通道
const int greenPin = 4;  // D2(GPIO4) - 绿色通道
const int bluePin = 0;   // D3(GPIO0) - 蓝色通道

// CA证书
static const char ca_cert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

// 灯光控制变量
String operationMode = "Manual";  // 操作模式: "Auto", "Manual", "Voice"
String lightMode = "Custom";      // 灯光模式: "Bright", "Warm", "Night", "Custom", "Auto", "Voice"
String voiceCommand = "None";     // 当前语音命令

// 语音模式RGB值
int voiceReddata = 0;
int voiceGreendata = 0;
int voiceBluedata = 0;

// 手动模式RGB值
int manualRedValue = 0;
int manualGreenValue = 0;
int manualBlueValue = 0;

// 预设灯光模式的RGB值
const int BRIGHT_MODE_R = 0, BRIGHT_MODE_G = 0, BRIGHT_MODE_B = 0;        // 明亮模式
const int WARM_MODE_R = 0, WARM_MODE_G = 75, WARM_MODE_B = 155;           // 温暖模式
const int NIGHT_MODE_R = 205, NIGHT_MODE_G = 205, NIGHT_MODE_B = 105;     // 夜灯模式

// 状态变量
unsigned long lastStatusUpdate = 0;
unsigned long lastReconnectAttempt = 0;
bool statusChanged = false;

// MQTT客户端设置
BearSSL::WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);

// 函数声明
void setLEDColor(int red, int green, int blue);
void adjustLightingBasedOnSensor();
void checkVoiceCommands();
void setOperationMode(const String& mode);
void setLightMode(const String& mode);
void sendStatusUpdate();

// JSON处理函数
String createJsonResponse() {
  if (operationMode == "Manual") {
    return ConverseDataToJson(
      "OperationMode", operationMode,
      "LightMode", lightMode,
      "Red", manualRedValue,
      "Green", manualGreenValue,
      "Blue", manualBlueValue
    );
  } 
  else if (operationMode == "Voice") {
    return ConverseDataToJson(
      "OperationMode", operationMode,
      "LightMode", lightMode,
      "VoiceCommand", voiceCommand,
      "Red", voiceReddata,
      "Green", voiceGreendata,
      "Blue", voiceBluedata
    );
  }
  else {
    // 自动模式或其他模式
    return ConverseDataToJson(
      "OperationMode", operationMode,
      "LightMode", lightMode
    );
  }
}

void setup() {
  // 初始化串口通信
  Serial.begin(9600);  // 用于语音模块和调试
  Serial.println("\nESP8266智能灯光系统启动...");
  
  // 设置引脚模式
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  
  // 初始化灯光(关闭状态)
  setLEDColor(0, 0, 0);
  
  // 连接WiFi
  connectToWiFi();
  
  // 同步网络时间(SSL需要)
  syncTime();
  
  // 设置MQTT服务器
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  
  // 连接MQTT服务器
  connectToMQTTBroker();
  
  Serial.println("系统初始化完成");
}

void loop() {
  // 检查MQTT连接
  if (!mqtt_client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      Serial.println("MQTT断开连接，尝试重连...");
      
      // 尝试重新连接
      if (connectToMQTTBroker()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    mqtt_client.loop();
  }
  
  // 根据操作模式执行不同的逻辑
  if (operationMode == "Auto") {
    adjustLightingBasedOnSensor();
  } 
  else if (operationMode == "Voice") {
    checkVoiceCommands();
  }
  
  // 状态变化时发送更新
  if (statusChanged && (millis() - lastStatusUpdate > 1000)) {
    sendStatusUpdate();
    statusChanged = false;
    lastStatusUpdate = millis();
  }
  
  // 短暂延时，减轻处理负担
  delay(50);
  yield(); // 允许ESP8266处理WiFi任务
}

// WiFi连接
void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("连接WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n已连接到WiFi网络: " + String(ssid));
  Serial.print("IP地址: ");
  Serial.println(WiFi.localIP());
}

// 时间同步
void syncTime() {
  configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
  Serial.print("等待NTP时间同步: ");
  while (time(nullptr) < 8 * 3600 * 2) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("时间已同步");
  
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.print("当前时间: ");
    Serial.println(asctime(&timeinfo));
  }
}

// 连接MQTT服务器
bool connectToMQTTBroker() {
  BearSSL::X509List serverTrustedCA(ca_cert);
  espClient.setTrustAnchors(&serverTrustedCA);
  
  String client_id = "esp8266-client-" + String(WiFi.macAddress());
  Serial.printf("连接到MQTT服务器，客户端ID: %s.....\n", client_id.c_str());
  
  if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
    Serial.println("已连接到MQTT服务器");
    
    // 订阅控制主题
    mqtt_client.subscribe(mqtt_topic);
    Serial.println("已订阅主题: " + String(mqtt_topic));
    
    // 发布连接成功消息
    String connectMsg = createJsonResponse();
    mqtt_client.publish(mqtt_status_topic, connectMsg.c_str(), true); // 保留消息
    
    return true;
  } else {
    char err_buf[128];
    espClient.getLastSSLError(err_buf, sizeof(err_buf));
    Serial.print("连接MQTT服务器失败, rc=");
    Serial.println(mqtt_client.state());
    Serial.println("SSL错误: ");
    Serial.println(err_buf);
    
    return false;
  }
}

// MQTT消息回调
void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.print("接收到MQTT消息，主题: ");
  Serial.println(topic);
  Serial.print("消息内容: ");
  
  // 创建一个字符串来保存消息内容
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
    message += (char) payload[i];
  }
  Serial.println();
  
  // 解析MQTT消息
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (!error) {
    bool stateChanged = false;
    
    // 检查是否包含操作模式命令
    if (doc.containsKey("OperationMode")) {
      String modeValue = doc["OperationMode"].as<String>();
      String oldMode = operationMode;
      
      setOperationMode(modeValue);
      
      if (oldMode != operationMode) {
        stateChanged = true;
      }
    }
    
    // 检查是否包含灯光模式命令 (仅在手动模式下有效)
    if (doc.containsKey("LightMode") && operationMode == "Manual") {
      String modeValue = doc["LightMode"].as<String>();
      
      String oldMode = lightMode;
      setLightMode(modeValue);
      
      if (oldMode != lightMode) {
        stateChanged = true;
      }
    }
    
    // 检查是否包含手动RGB设置 (仅在手动模式下有效)
    if (operationMode == "Manual" && 
       (doc.containsKey("Red") || doc.containsKey("Green") || doc.containsKey("Blue"))) {
      
      if (lightMode != "Custom") {
        lightMode = "Custom";
        stateChanged = true;
      }
      
      if (doc.containsKey("Red")) {
        int oldRed = manualRedValue;
        manualRedValue = doc["Red"].as<int>();
        if (oldRed != manualRedValue) stateChanged = true;
      }
      if (doc.containsKey("Green")) {
        int oldGreen = manualGreenValue;
        manualGreenValue = doc["Green"].as<int>();
        if (oldGreen != manualGreenValue) stateChanged = true;
      }
      if (doc.containsKey("Blue")) {
        int oldBlue = manualBlueValue;
        manualBlueValue = doc["Blue"].as<int>();
        if (oldBlue != manualBlueValue) stateChanged = true;
      }
      
      // 应用手动设置的RGB值
      setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
    }
    
    // 仅当状态发生变化时才标记为需要更新
    if (stateChanged) {
      statusChanged = true;
    }
  }
}

// 设置LED颜色 - 适应ESP8266的0-1023 PWM范围
// 设置LED颜色 - 适应ESP8266的0-1023 PWM范围（共阳极LED版本）
void setLEDColor(int red, int green, int blue) {
  // 将0-255范围映射到0-1023范围
  int espRed = map(red, 0, 255, 0, 255);
  int espGreen = map(green, 0, 255, 0, 255);
  int espBlue = map(blue, 0, 255, 0, 255);
  
  // 对于共阳极LED，不需要反转逻辑
  // 共阳极LED已经是高电平熄灭，低电平点亮
  
  analogWrite(redPin, espRed);
  analogWrite(greenPin, espGreen);
  analogWrite(bluePin, espBlue);
}


// 设置操作模式
void setOperationMode(const String& mode) {
  String oldMode = operationMode;
  
  if (mode == "Auto") {
    operationMode = "Auto";
    lightMode = "Auto";
    Serial.println("已切换到自动调节模式");
  } 
  else if (mode == "Manual") {
    operationMode = "Manual";
    // 进入手动模式时保持当前RGB设置，但如果之前是Auto或Voice则设为Custom
    if (oldMode == "Auto" || oldMode == "Voice") {
      lightMode = "Custom";
    }
    Serial.println("已切换到手动控制模式");
  }
  else if (mode == "Voice") {
    operationMode = "Voice";
    lightMode = "Voice";
    voiceCommand = "None";
    Serial.println("已切换到语音控制模式");
  }
  
  // 如果切换到手动模式，应用当前的手动RGB设置
  if (operationMode == "Manual") {
    setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
  }
  
  // 如果模式发生变化，标记需要更新状态
  if (oldMode != operationMode) {
    statusChanged = true;
  }
}

// 设置预设灯光模式
void setLightMode(const String& mode) {
  String oldMode = lightMode;
  lightMode = mode;
  
  if (mode == "Bright") {
    // 明亮模式 - 纯白光
    manualRedValue = BRIGHT_MODE_R;
    manualGreenValue = BRIGHT_MODE_G;
    manualBlueValue = BRIGHT_MODE_B;
  } else if (mode == "Warm") {
    // 温暖模式 - 暖黄色光
    manualRedValue = WARM_MODE_R;
    manualGreenValue = WARM_MODE_G;
    manualBlueValue = WARM_MODE_B;
  } else if (mode == "Night") {
    // 夜灯模式 - 柔和蓝光
    manualRedValue = NIGHT_MODE_R;
    manualGreenValue = NIGHT_MODE_G;
    manualBlueValue = NIGHT_MODE_B;
  } else {
    // 未知模式，保持当前值不变
    lightMode = "Custom";
  }
  
  // 应用新的灯光设置
  setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
  
  Serial.print("已设置灯光模式: ");
  Serial.println(lightMode);
  
  // 如果模式发生变化，标记需要更新状态
  if (oldMode != lightMode) {
    statusChanged = true;
  }
}

// 基于光敏传感器调整灯光
void adjustLightingBasedOnSensor() {
  // 读取光敏模块数值（0~1023）
  int sensorValue = analogRead(photoSensorPin);
  
  // 将传感器值转换为百分比（0~100%）
  int percentage = map(sensorValue, 0, 1023, 100, 0);
  
  // 计算PWM值
  int pwmValue = map(sensorValue, 0, 1023, 255, 0);
  
  // 微调，确保在完全黑暗时灯光最亮
  if (percentage > 90) {
    pwmValue = 255; // 最亮（反转逻辑）
  }
  
  // 确保PWM值在有效范围内
  if (pwmValue < 0) pwmValue = 0;
  if (pwmValue > 255) pwmValue = 255;
  
  // 设置RGB值（白光）
  setLEDColor(pwmValue, pwmValue, pwmValue);
}

// 检查语音命令
// 检查语音命令 - 完整移植所有语音命令
void checkVoiceCommands() {
  if (Serial.available() >= 4) {
    byte receivedData[4];
    Serial.readBytes(receivedData, 4);
  
    String oldCommand = voiceCommand;
    String commandHex = "";
    
    for (int i = 0; i < 4; i++) {
      char hexStr[3];
      sprintf(hexStr, "%02X", receivedData[i]);
      commandHex += hexStr;
    }
    
    Serial.print("收到语音指令(HEX): ");
    Serial.println(commandHex);
    
    // 判断接收到的命令
    if (receivedData[0] == 0xA3 && receivedData[1] == 0xA3 &&
        receivedData[2] == 0xA3 && receivedData[3] == 0xA3) {
      voiceReddata = 0;
      voiceGreendata = 0;
      voiceBluedata = 0;
      setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
      voiceCommand = "White";
      lightMode = "Voice-white";
      Serial.println("语音命令：开启客厅灯");
    }
    else if (receivedData[0] == 0xA4 && receivedData[1] == 0xA4 &&
             receivedData[2] == 0xA4 && receivedData[3] == 0xA4) {
      voiceReddata = 255;
      voiceGreendata = 255;
      voiceBluedata = 255;
      setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
      voiceCommand = "Black";
      lightMode = "Voice-Black";
      Serial.println("语音命令:关闭客厅灯");
    }
    else if (receivedData[0] == 0xA7 && receivedData[1] == 0xA7 &&
             receivedData[2] == 0xA7 && receivedData[3] == 0xA7) {
      // 亮度调高逻辑
      if(voiceReddata == voiceGreendata && voiceBluedata == voiceGreendata && voiceReddata >= 20) {
        voiceReddata -= 40;
        voiceGreendata -= 40;
        voiceBluedata -= 40;
      }
      else if(voiceReddata == voiceGreendata && voiceBluedata == voiceGreendata && voiceReddata < 20) {
        voiceReddata = 0;
        voiceGreendata = 0;
        voiceBluedata = 0;
      }
      else {
        voiceReddata *= 0.8;
        voiceGreendata *= 0.8;
        voiceBluedata *= 0.8;
      }
      setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
      voiceCommand = "light up";
      lightMode = "Voice-up";
      Serial.println("语音命令：亮度调高");
    }
    else if (receivedData[0] == 0xA8 && receivedData[1] == 0xA8 &&
             receivedData[2] == 0xA8 && receivedData[3] == 0xA8) {
      // 亮度调低逻辑
      if(voiceReddata == voiceGreendata && voiceBluedata == voiceGreendata && voiceReddata <= 235) {
        voiceReddata += 40;
        voiceGreendata += 40;
        voiceBluedata += 40;
      }
      else if(voiceReddata == voiceGreendata && voiceBluedata == voiceGreendata && voiceReddata > 235) {
        voiceReddata = 255;
        voiceGreendata = 255;
        voiceBluedata = 255;
      }
      else if (voiceReddata <= 212 && voiceGreendata <= 212 && voiceBluedata <= 212) {
        voiceReddata *= 1.2;
        voiceGreendata *= 1.2;
        voiceBluedata *= 1.2;
      }
      else {
        // 其他情况保持不变
        voiceReddata = voiceReddata;
        voiceGreendata = voiceGreendata;
        voiceBluedata = voiceBluedata;
      }
      setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
      voiceCommand = "light down";
      lightMode = "Voice-down";
      Serial.println("语音命令：亮度调低");
    }
    else if (receivedData[0] == 0xA9 && receivedData[1] == 0xA9 &&
             receivedData[2] == 0xA9 && receivedData[3] == 0xA9) {
      voiceReddata = 0;
      voiceGreendata = 75;
      voiceBluedata = 155;
      setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
      voiceCommand = "light Soft";
      lightMode = "Voice-soft";
      Serial.println("语音命令：打开柔光灯");
    }
    // 阅读模式开启
    else if (receivedData[0] == 0x10 && receivedData[1] == 0x10 &&
             receivedData[2] == 0x10 && receivedData[3] == 0x10) {
      voiceReddata = 0;
      voiceGreendata = 75;
      voiceBluedata = 155;
      setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
      voiceCommand = "Read model";
      lightMode = "Voice-read";
      Serial.println("语音命令：打开阅读模式");
    }
    // 阅读模式关闭
    else if (receivedData[0] == 0x11 && receivedData[1] == 0x11 &&
             receivedData[2] == 0x11 && receivedData[3] == 0x11) {
      voiceReddata = 255;
      voiceGreendata = 255;
      voiceBluedata = 255;
      setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
      voiceCommand = "Read model-c";
      lightMode = "Voice-read-c";
      Serial.println("语音命令：关闭阅读模式");
    }
    // 睡眠模式开启
    else if (receivedData[0] == 0x12 && receivedData[1] == 0x12 &&
             receivedData[2] == 0x12 && receivedData[3] == 0x12) {
      voiceReddata = 60;
      voiceGreendata = 120;
      voiceBluedata = 230;
      setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
      voiceCommand = "Sleep model";
      lightMode = "Voice-sleep";
      Serial.println("语音命令：打开睡眠模式");
    }
    // 睡眠模式关闭
    else if (receivedData[0] == 0x13 && receivedData[1] == 0x13 &&
             receivedData[2] == 0x13 && receivedData[3] == 0x13) {
      voiceReddata = 255;
      voiceGreendata = 255;
      voiceBluedata = 255;
      setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
      voiceCommand = "Sleep model-c";
      lightMode = "Voice-sleep-c";
      Serial.println("语音命令：关闭睡眠模式");
    }
    else {
      // 无效命令 - 不改变当前状态
      Serial.println("无效的语音命令");
    }
    
    // 如果命令有变化，标记需要上报状态
    if (oldCommand != voiceCommand) {
      statusChanged = true;
    }
  }
}


// 发送状态更新到MQTT
void sendStatusUpdate() {
  String statusJson = createJsonResponse();
  
  if (mqtt_client.connected()) {
    mqtt_client.publish(mqtt_status_topic, statusJson.c_str(), true);
    Serial.println("已发布状态更新: " + statusJson);
  } else {
    Serial.println("MQTT未连接，无法发布状态");
  }
  
  lastStatusUpdate = millis();
}
