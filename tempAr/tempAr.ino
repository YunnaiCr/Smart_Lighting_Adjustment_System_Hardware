#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include "JsonFactory.h" // 请确保您有这个头文件和对应的实现

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
const long gmt_offset_sec = 28800; // GMT+8
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
void mqttCallback(char *topic, byte *payload, unsigned int length);
void connectToWiFi();
void syncTime();
bool connectToMQTTBroker();


// JSON处理函数 (使用上面提供的简单实现或您自己的 JsonFactory)
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
  else { // 自动模式或其他模式
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
  setLEDColor(0, 0, 0); // 假设0,0,0是灯灭（或者您期望的初始最亮白色，取决于共阳/共阴以及setLEDColor逻辑）
                        // 如果 BRIGHT_MODE_R,G,B = 0,0,0 是最亮白，这里设为255,255,255可能是灯灭
                        // 根据您的硬件和setLEDColor实际行为调整

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
    if (now - lastReconnectAttempt > 5000) { // 5秒尝试重连一次
      lastReconnectAttempt = now;
      Serial.println("MQTT断开连接，尝试重连...");

      // 尝试重新连接
      if (connectToMQTTBroker()) {
        lastReconnectAttempt = 0; // 重置计时器
      }
    }
  } else {
    mqtt_client.loop(); // 处理MQTT消息
  }

  // 根据操作模式执行不同的逻辑
  if (operationMode == "Auto") {
    adjustLightingBasedOnSensor();
  }
  else if (operationMode == "Voice") {
    checkVoiceCommands();
  }
  // 手动模式的灯光控制主要由MQTT回调中的setLightMode和RGB直接设置驱动

  // 状态变化时发送更新 (每秒最多一次)
  if (statusChanged && (millis() - lastStatusUpdate > 1000)) {
    sendStatusUpdate();
    // statusChanged = false; // sendStatusUpdate 内部会重置
    // lastStatusUpdate = millis(); // sendStatusUpdate 内部会更新
  }

  // 短暂延时，减轻处理负担
  delay(50);
  yield(); // 允许ESP8266处理WiFi和其他后台任务
}

// WiFi连接
void connectToWiFi() {
  WiFi.mode(WIFI_STA); // 设置为STA模式
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
  time_t now = time(nullptr);
  // 等待时间戳大于某个合理的值，表示时间已同步 (例如：大于2020年的某个时间戳)
  // 8 * 3600 * 2 (16 hours) seems a bit arbitrary, a common check is (now < 1000000000) for uninitialized time
  while (now < 1672531200) { // 1672531200 is Jan 1, 2023 timestamp. Ensures time is somewhat sane.
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\n时间已同步");

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.print("当前时间: ");
    Serial.println(asctime(&timeinfo));
  } else {
    Serial.println("获取本地时间失败");
  }
}

// 连接MQTT服务器
bool connectToMQTTBroker() {
  // BearSSL 需要时间同步，确保 syncTime() 在此之前被调用
  BearSSL::X509List serverTrustedCA(ca_cert);
  espClient.setTrustAnchors(&serverTrustedCA);
  // 对于不安全的连接 (mqtt_port = 1883)，不需要设置证书
  // espClient.setInsecure(); // 如果不使用SSL/TLS

  String client_id = "esp8266-client-" + String(WiFi.macAddress());
  Serial.printf("连接到MQTT服务器 %s:%d，客户端ID: %s\n", mqtt_broker, mqtt_port, client_id.c_str());

  // 使用用户名和密码连接
  if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
    Serial.println("已连接到MQTT服务器");

    // 订阅控制主题
    if (mqtt_client.subscribe(mqtt_topic)) {
        Serial.println("已订阅主题: " + String(mqtt_topic));
    } else {
        Serial.println("订阅主题失败!");
    }

    // 发布连接成功/初始状态消息
    statusChanged = true; // 标记状态已变化，以便发送初始状态
    // sendStatusUpdate(); // 立即发送或等待loop中发送
    return true;
  } else {
    Serial.print("连接MQTT服务器失败, rc=");
    Serial.print(mqtt_client.state());
    char err_buf[128];
    espClient.getLastSSLError(err_buf, sizeof(err_buf)); // 获取SSL错误信息
    Serial.println("\nSSL错误: " + String(err_buf)); // 打印SSL错误
    return false;
  }
}

// MQTT消息回调
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("接收到MQTT消息，主题: [");
  Serial.print(topic);
  Serial.print("] ");

  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("消息内容: " + message);

  StaticJsonDocument<256> doc; // 适当调整JSON文档大小
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  bool stateChangedThisCallback = false;

  // 检查是否包含操作模式命令
  if (doc.containsKey("OperationMode")) {
    String newOpMode = doc["OperationMode"].as<String>();
    if (operationMode != newOpMode) { // 仅当模式实际改变时才处理
      setOperationMode(newOpMode); // setOperationMode内部会设置statusChanged
      stateChangedThisCallback = true; // 标记本次回调中有状态改变
    }
  }

  // 检查是否包含灯光模式命令 (仅在手动模式下有效)
  if (doc.containsKey("LightMode") && operationMode == "Manual") {
    String newLightMode = doc["LightMode"].as<String>();
    if (lightMode != newLightMode || newLightMode != "Custom") { // 如果模式改变，或者不是Custom（因为Custom可能只是RGB变了）
        // 对于 "Custom" 模式，其RGB值通过下面的 Red, Green, Blue 来设置
        // 如果 MQTT 发送 "LightMode": "Custom"，我们只更新 lightMode 变量，
        // 实际的颜色变化依赖于是否同时发送了 R,G,B 值。
        // 如果只发送了预设模式 (Bright, Warm, Night)，则 setLightMode 会应用颜色。
        if (newLightMode == "Bright" || newLightMode == "Warm" || newLightMode == "Night") {
            setLightMode(newLightMode); // setLightMode内部会设置statusChanged和应用颜色
            stateChangedThisCallback = true;
        } else if (newLightMode == "Custom") {
            if (lightMode != "Custom") {
                lightMode = "Custom"; // 只更新模式变量
                stateChangedThisCallback = true; // 模式变量改变了
            }
        }
    }
  }

  // 检查是否包含手动RGB设置 (仅在手动模式 "Manual" 且灯光模式为 "Custom" 或通过RGB命令隐式变为 "Custom")
  if (operationMode == "Manual" && (doc.containsKey("Red") || doc.containsKey("Green") || doc.containsKey("Blue"))) {
    bool colorChanged = false;
    if (lightMode != "Custom") { // 如果当前不是Custom模式，收到RGB值意味着切换到Custom模式
      lightMode = "Custom";
      stateChangedThisCallback = true; // lightMode 变量改变
    }

    if (doc.containsKey("Red")) {
      int newRed = doc["Red"].as<int>();
      if (manualRedValue != newRed) {
        manualRedValue = newRed;
        colorChanged = true;
      }
    }
    if (doc.containsKey("Green")) {
      int newGreen = doc["Green"].as<int>();
      if (manualGreenValue != newGreen) {
        manualGreenValue = newGreen;
        colorChanged = true;
      }
    }
    if (doc.containsKey("Blue")) {
      int newBlue = doc["Blue"].as<int>();
      if (manualBlueValue != newBlue) {
        manualBlueValue = newBlue;
        colorChanged = true;
      }
    }

    if (colorChanged) {
      setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
      stateChangedThisCallback = true; // RGB值或模式改变
    }
  }

  if (stateChangedThisCallback) {
    statusChanged = true; // 设置全局状态变化标志，以便在loop中发送MQTT更新
  }
}



void setLEDColor(int red, int green, int blue) {


  int espRed = map(red, 0, 255, 0, 255);
  int espGreen = map(green, 0, 255, 0, 255);
  int espBlue = map(blue, 0, 255, 0, 255);

  analogWrite(redPin, espRed);
  analogWrite(greenPin, espGreen);
  analogWrite(bluePin, espBlue);

  // 调试信息
  // Serial.printf("Set LED Color: R=%d, G=%d, B=%d (Mapped: R=%d, G=%d, B=%d)\n", red, green, blue, espRed, espGreen, espBlue);
}


// 设置操作模式 (已修改，增加了串口指令发送)
void setOperationMode(const String& newMode) {
  String oldOperationMode = operationMode; // 保存旧模式，用于判断是否真的切换了

  if (oldOperationMode == newMode) {
    // Serial.println("操作模式未改变: " + newMode);
    return; // 模式未改变，则不执行任何操作
  }

  // 检查是否从语音模式切换出去
  if (oldOperationMode == "Voice" && newMode != "Voice") {
    byte exitVoiceCmd[] = {0xAA, 0x01, 0x01, 0x00, 0x00, 0x00, 0x55, 0xAA};
    Serial.write(exitVoiceCmd, sizeof(exitVoiceCmd));
    Serial.println("串口发送：退出语音模式指令");
  }
  // 检查是否切换到语音模式
  else if (oldOperationMode != "Voice" && newMode == "Voice") {
    byte enterVoiceCmd[] = {0xAA, 0x02, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA};
    Serial.write(enterVoiceCmd, sizeof(enterVoiceCmd));
    Serial.println("串口发送：进入语音模式指令");
  }

  // 更新操作模式
  operationMode = newMode;

  if (newMode == "Auto") {
    lightMode = "Auto"; // 自动模式下，灯光模式也设为Auto
    Serial.println("已切换到自动调节模式");
  }
  else if (newMode == "Manual") {
    // 进入手动模式时，如果之前是Auto或Voice，则灯光模式设为Custom，并应用当前手动RGB值
    // 否则保持之前的灯光模式 (例如之前是 Bright, Warm, Night)
    if (oldOperationMode == "Auto" || oldOperationMode == "Voice") {
      lightMode = "Custom";
    }
    Serial.println("已切换到手动控制模式");
    // 切换到手动模式时，应用当前的手动RGB值
    setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
  }
  else if (newMode == "Voice") {
    lightMode = "Voice"; // 语音模式下，灯光模式设为Voice
    voiceCommand = "None"; // 重置语音命令状态
    Serial.println("已切换到语音控制模式");
    // 语音模式的灯光由 checkVoiceCommands 函数控制
  }
  else {
    Serial.println("设置了未知的操作模式: " + newMode);
    operationMode = oldOperationMode; // 恢复旧模式，因为新模式未知
    return; // 不标记状态改变
  }

  statusChanged = true; // 标记状态已改变
}


// 设置预设灯光模式 (仅在手动模式下由MQTT调用，或内部调用)
void setLightMode(const String& mode) {
  String oldLightM = lightMode; // 保存旧的灯光模式

  // 此函数仅应在 operationMode == "Manual" 时被有效调用来改变灯光
  // 如果当前不是手动模式，设置预设灯光模式可能没有意义，除非是为了在切换到手动时准备一个状态
  if (operationMode != "Manual") {
    // Serial.println("警告: 尝试在非手动模式下设置灯光模式 (" + mode + "). 操作模式: " + operationMode);
    // 可以选择直接返回，或者仅更新 lightMode 变量但不应用颜色，等待切换到手动模式
    // 为了符合您之前的逻辑，这里只更新变量和颜色，即使不在手动模式（尽管通常不这么做）
  }

  bool applied = false;
  if (mode == "Bright") {
    manualRedValue = BRIGHT_MODE_R;
    manualGreenValue = BRIGHT_MODE_G;
    manualBlueValue = BRIGHT_MODE_B;
    lightMode = mode;
    applied = true;
  } else if (mode == "Warm") {
    manualRedValue = WARM_MODE_R;
    manualGreenValue = WARM_MODE_G;
    manualBlueValue = WARM_MODE_B;
    lightMode = mode;
    applied = true;
  } else if (mode == "Night") {
    manualRedValue = NIGHT_MODE_R;
    manualGreenValue = NIGHT_MODE_G;
    manualBlueValue = NIGHT_MODE_B;
    lightMode = mode;
    applied = true;
  } else if (mode == "Custom") {
    // 当明确设置为 "Custom" 时，我们不改变当前的 manualRedValue 等值，
    // 这些值应该由单独的 R, G, B 命令设置。
    lightMode = "Custom";
    // applied = false; // 因为颜色值没有在这里改变
  } else {
    Serial.println("未知的灯光模式: " + mode + "，灯光模式保持为 " + lightMode);
    // lightMode = "Custom"; // 或者可以强制设为Custom
    return; // 不改变状态，不应用未知模式
  }

  if (applied) {
    setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
    Serial.println("已设置灯光模式: " + lightMode);
  } else if (oldLightM != lightMode) { // 例如从 Bright 切换到 Custom (仅模式名改变)
     Serial.println("灯光模式已更改为: " + lightMode + " (颜色值未在此处更新)");
  }


  // 如果灯光模式的名称发生变化，或应用了新的预设颜色，标记需要更新状态
  if (oldLightM != lightMode || applied) {
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


// 检查语音命令 (仅在 operationMode == "Voice" 时由 loop 调用)
void checkVoiceCommands() {
  if (Serial.available() >= 4) { // 确保至少有4个字节的数据
    byte receivedData[4];
    Serial.readBytes(receivedData, 4); // 读取4个字节

    String oldVoiceCmd = voiceCommand; // 保存旧的语音命令
    String oldLightM = lightMode;    // 保存旧的灯光模式 (voice-xxx)

    String commandHex = "";
    for (int i = 0; i < 4; i++) {
      char hexStr[3];
      sprintf(hexStr, "%02X", receivedData[i]); // 正确格式化为两位十六进制数
      commandHex += hexStr;
    }

    Serial.print("收到语音指令(HEX): ");
    Serial.println(commandHex);

    bool commandProcessed = false;

    // 判断接收到的命令 (根据您提供的逻辑)
    if (receivedData[0] == 0xA3 && receivedData[1] == 0xA3 &&
        receivedData[2] == 0xA3 && receivedData[3] == 0xA3) { // 开启客厅灯 (白光)
      voiceReddata = 0;   // 对应 BRIGHT_MODE_R
      voiceGreendata = 0; // 对应 BRIGHT_MODE_G
      voiceBluedata = 0;  // 对应 BRIGHT_MODE_B
      voiceCommand = "White";
      lightMode = "Voice-white";
      Serial.println("语音命令：开启客厅灯");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0xA4 && receivedData[1] == 0xA4 &&
             receivedData[2] == 0xA4 && receivedData[3] == 0xA4) { // 关闭客厅灯
      voiceReddata = 255; // 假设255,255,255是灯灭 (如果0,0,0是最亮白)
      voiceGreendata = 255;
      voiceBluedata = 255;
      voiceCommand = "Black";
      lightMode = "Voice-Black";
      Serial.println("语音命令:关闭客厅灯");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0xA7 && receivedData[1] == 0xA7 &&
             receivedData[2] == 0xA7 && receivedData[3] == 0xA7) { // 亮度调高
      // 您的亮度调高逻辑 (减少RGB值，假设值越小越亮)
      if(voiceReddata == voiceGreendata && voiceBluedata == voiceGreendata) { // 灰度/白光
        int currentBrightness = voiceReddata; // 假设三者相等
        currentBrightness = max(0, currentBrightness - 40); // 减少40，但不小于0
        voiceReddata = voiceGreendata = voiceBluedata = currentBrightness;
      } else { // 彩色光
        voiceReddata = max(0, (int)(voiceReddata * 0.8));
        voiceGreendata = max(0, (int)(voiceGreendata * 0.8));
        voiceBluedata = max(0, (int)(voiceBluedata * 0.8));
      }
      voiceCommand = "light up";
      lightMode = "Voice-up";
      Serial.println("语音命令：亮度调高");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0xA8 && receivedData[1] == 0xA8 &&
             receivedData[2] == 0xA8 && receivedData[3] == 0xA8) { // 亮度调低
      // 您的亮度调低逻辑 (增加RGB值，假设值越大越暗)
       if(voiceReddata == voiceGreendata && voiceBluedata == voiceGreendata) { // 灰度/白光
        int currentBrightness = voiceReddata;
        currentBrightness = min(255, currentBrightness + 40); // 增加40，但不大于255
        voiceReddata = voiceGreendata = voiceBluedata = currentBrightness;
      } else { // 彩色光
         // 之前的 voiceReddata *= 1.2 可能导致值很快超过255。使用 min 确保。
        voiceReddata = min(255, (int)(voiceReddata / 0.8)); // 之前是 *1.2，除以0.8更精确对应上面的*0.8
        voiceGreendata = min(255, (int)(voiceGreendata / 0.8));
        voiceBluedata = min(255, (int)(voiceBluedata / 0.8));
      }
      voiceCommand = "light down";
      lightMode = "Voice-down";
      Serial.println("语音命令：亮度调低");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0xA9 && receivedData[1] == 0xA9 &&
             receivedData[2] == 0xA9 && receivedData[3] == 0xA9) { // 打开柔光灯 (暖光)
      voiceReddata = WARM_MODE_R;
      voiceGreendata = WARM_MODE_G;
      voiceBluedata = WARM_MODE_B;
      voiceCommand = "light Soft";
      lightMode = "Voice-soft";
      Serial.println("语音命令：打开柔光灯");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0x10 && receivedData[1] == 0x10 &&
             receivedData[2] == 0x10 && receivedData[3] == 0x10) { // 阅读模式开启 (暖光)
      voiceReddata = WARM_MODE_R; // 与柔光灯相同
      voiceGreendata = WARM_MODE_G;
      voiceBluedata = WARM_MODE_B;
      voiceCommand = "Read model";
      lightMode = "Voice-read";
      Serial.println("语音命令：打开阅读模式");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0x11 && receivedData[1] == 0x11 &&
             receivedData[2] == 0x11 && receivedData[3] == 0x11) { // 阅读模式关闭 (灯灭)
      voiceReddata = 255;
      voiceGreendata = 255;
      voiceBluedata = 255;
      voiceCommand = "Read model-c";
      lightMode = "Voice-read-c";
      Serial.println("语音命令：关闭阅读模式");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0x12 && receivedData[1] == 0x12 &&
             receivedData[2] == 0x12 && receivedData[3] == 0x12) { // 睡眠模式开启
      voiceReddata = 60;
      voiceGreendata = 120;
      voiceBluedata = 230;
      voiceCommand = "Sleep model";
      lightMode = "Voice-sleep";
      Serial.println("语音命令：打开睡眠模式");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0x13 && receivedData[1] == 0x13 &&
             receivedData[2] == 0x13 && receivedData[3] == 0x13) { // 睡眠模式关闭 (灯灭)
      voiceReddata = 255;
      voiceGreendata = 255;
      voiceBluedata = 255;
      voiceCommand = "Sleep model-c";
      lightMode = "Voice-sleep-c";
      Serial.println("语音命令：关闭睡眠模式");
      commandProcessed = true;
    }
    else {
      Serial.println("无效的语音命令");
      // 不改变当前灯光状态和命令
    }

    if (commandProcessed) {
      setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
      // 如果语音命令或对应的灯光模式发生变化，标记需要上报状态
      if (oldVoiceCmd != voiceCommand || oldLightM != lightMode) {
        statusChanged = true;
      }
    }
  }
}


// 发送状态更新到MQTT
void sendStatusUpdate() {
  if (!mqtt_client.connected()) {
    Serial.println("MQTT未连接，无法发布状态");
    statusChanged = false; // 重置标志，因为没有发送
    return;
  }

  String statusJson = createJsonResponse(); // 生成包含当前状态的JSON字符串

  if (mqtt_client.publish(mqtt_status_topic, statusJson.c_str(), true)) { // true表示保留消息
    Serial.println("已发布状态更新: " + statusJson);
  } else {
    Serial.println("状态更新发布失败!");
  }

  lastStatusUpdate = millis(); // 更新上次发送状态的时间戳
  statusChanged = false;       // 重置状态变化标志
}
