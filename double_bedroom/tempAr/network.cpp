#include "network.h"
#include <time.h>
#include <string.h>
#include <WiFiUdp.h>
#include "config.h"

BearSSL::WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);

// WiFi连接
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
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

String ipToString(IPAddress ip) {
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

// 时间同步
void syncTime() {
  configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
  Serial.print("等待NTP时间同步: ");
  time_t now = time(nullptr);
  while (now < 1672531200) {
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
  BearSSL::X509List serverTrustedCA(ca_cert);
  espClient.setTrustAnchors(&serverTrustedCA);
  String client_id = "esp8266-client-" + String(WiFi.macAddress());
  Serial.printf("连接到MQTT服务器 %s:%d，客户端ID: %s\n", mqtt_broker, mqtt_port, client_id.c_str());
  
  if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
    Serial.println("已连接到MQTT服务器");
    
    // 订阅命令和状态通道
    if (mqtt_client.subscribe(mqtt_esp_topic)) {
      Serial.println("已订阅命令通道: " + String(mqtt_esp_topic));
    } else {
      Serial.println("订阅命令通道失败!");
    }
    
    if (mqtt_client.subscribe(mqtt_esp_status_topic)) {
      Serial.println("已订阅状态通道: " + String(mqtt_esp_status_topic));
    } else {
      Serial.println("订阅状态通道失败!");
    }
    
    // 发送同步请求(通过状态通道发送字符串)
    if (mqtt_client.publish(mqtt_app_status_topic, "sync")) {
      Serial.println("已发送状态同步请求");
    } else {
      Serial.println("状态同步请求发送失败!");
    }
    
    return true;
  } else {
    Serial.print("连接MQTT服务器失败, rc=");
    Serial.print(mqtt_client.state());
    char err_buf[128];
    espClient.getLastSSLError(err_buf, sizeof(err_buf));
    Serial.println("\nSSL错误: " + String(err_buf));
    return false;
  }
}
