#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

// WiFi设置
const char *ssid = "wyxbupt";
const char *password = "wuyuxin155";

// MQTT Broker设置
const char *mqtt_broker = "jfe2a84f.ala.cn-hangzhou.emqxsl.cn";
const char *mqtt_topic = "light/livingroom";
const char *mqtt_status_topic = "light/livingroom/status"; // 状态反馈主题
const char *mqtt_username = "Yunnai";
const char *mqtt_password = "azathoth";
const int mqtt_port = 8883;

const char *ntp_server = "pool.ntp.org";
const long gmt_offset_sec = 28800;
const int daylight_offset_sec = 0;

// 定义软件串口 - D2(GPIO4)、D1(GPIO5)用于与Arduino通信
SoftwareSerial ArduinoSerial(4, 5); // RX, TX

BearSSL::WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);

// 上次尝试重连的时间
unsigned long lastReconnectAttempt = 0;
// 是否已查询过初始状态
bool initialStatusQueried = false;

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

void connectToWiFi();
bool connectToMQTTBroker();
void syncTime();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void sendToArduino(const String& command);
void sendArduinoResponseToMQTT(const String& response);
void sendStatusQuery();
String createJsonMessage(const char* key, const String& value);

void setup() {
  // 初始化硬件串口(用于调试输出)
  Serial.begin(115200);
  Serial.println("\nESP8266启动中...");
  
  // 初始化软件串口(用于与Arduino通信)
  ArduinoSerial.begin(9600);
  
  connectToWiFi();
  syncTime();  // X.509验证需要同步时间
  
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  
  connectToMQTTBroker();
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  delay(10);
  Serial.print("连接WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n已连接到WiFi网络: " + String(ssid));
  Serial.print("IP地址: ");
  Serial.println(WiFi.localIP());
}

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
  } else {
    Serial.println("获取本地时间失败.");
  }
}

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
    String connectMsg = createJsonMessage("status", "ESP8266已连接，正在查询Arduino状态");
    mqtt_client.publish(mqtt_status_topic, connectMsg.c_str(), true); // 保留消息
    
    // 连接成功后，立即查询Arduino状态
    sendStatusQuery();
    initialStatusQueried = true;
    
    return true;
  } else {
    char err_buf[128];
    espClient.getLastSSLError(err_buf, sizeof(err_buf));
    Serial.print("连接MQTT服务器失败, rc=");
    Serial.println(mqtt_client.state());
    Serial.println("SSL错误: ");
    Serial.println(err_buf);
    Serial.println("5秒后重试");
    
    return false;
  }
}

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
    // 检查是否包含OperationMode命令
    if (doc.containsKey("OperationMode")) {
      String mode = doc["OperationMode"].as<String>();
      
      // 创建发送给Arduino的命令
      StaticJsonDocument<100> cmdDoc;
      cmdDoc["OperationMode"] = mode;
      
      String arduinoCmd;
      serializeJson(cmdDoc, arduinoCmd);
      
      // 发送操作模式命令到Arduino
      sendToArduino(arduinoCmd);
      Serial.println("已发送操作模式命令: " + mode);
      
      return;
    }
    
    // 检查是否包含LightMode命令
    if (doc.containsKey("LightMode")) {
      String mode = doc["LightMode"].as<String>();
      
      // 创建发送给Arduino的命令
      StaticJsonDocument<100> cmdDoc;
      cmdDoc["LightMode"] = mode;
      
      String arduinoCmd;
      serializeJson(cmdDoc, arduinoCmd);
      
      // 发送灯光模式命令到Arduino
      sendToArduino(arduinoCmd);
      Serial.println("已发送灯光模式命令: " + mode);
      
      return;
    }
  }
  
  // 如果没有特殊处理的命令，直接将MQTT消息转发给Arduino
  sendToArduino(message);
  
  Serial.println("--------------------------------");
}

// 发送命令到Arduino
void sendToArduino(const String& command) {
  Serial.println("向Arduino发送命令: " + command);
  ArduinoSerial.println(command);
  delay(50); // 给Arduino一点时间处理
}

// 查询Arduino当前状态
void sendStatusQuery() {
  Serial.println("查询Arduino当前状态...");
  String queryCommand = "{\"Query\":\"Status\"}";
  sendToArduino(queryCommand);
}

// 创建简单的JSON消息
String createJsonMessage(const char* key, const String& value) {
  StaticJsonDocument<200> doc;
  doc[key] = value;
  
  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

// 将Arduino响应发送回MQTT
void sendArduinoResponseToMQTT(const String& response) {
  if (mqtt_client.connected()) {
    // 将response设置为保留消息(第三个参数为true)
    mqtt_client.publish(mqtt_status_topic, response.c_str(), true);
    Serial.println("Arduino状态已发布到MQTT(保留): " + response);
  } else {
    Serial.println("MQTT未连接，无法发布状态");
  }
}

// 在loop函数中修改接收Arduino响应的部分

void loop() {
  // 检查MQTT连接状态
  if (!mqtt_client.connected()) {
    // ... 现有代码保持不变 ...
  } else {
    mqtt_client.loop();
  }
  
  // 检查来自Arduino的消息
  if (ArduinoSerial.available()) {
    String response = ArduinoSerial.readStringUntil('\n');
    response.trim();
    
    Serial.println("收到原始响应: [" + response + "]"); // 添加调试输出
    
    // 忽略空响应和非JSON响应
    if (response.length() < 2 || response[0] != '{') {
      Serial.println("忽略无效响应");
      return;
    }
    
    // 检查是否是有效的JSON格式
    StaticJsonDocument<300> doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      Serial.println("收到Arduino响应: " + response);
      
      // 将Arduino的响应发送回MQTT (设置为保留消息)
      sendArduinoResponseToMQTT(response);
    } else {
      Serial.print("JSON解析失败: ");
      Serial.println(error.c_str());
      Serial.println("问题响应: [" + response + "]");
    }
  }
  
  // 延时减轻处理负担
  delay(50);
}
