#include "config.h"

// WiFi设置
const char *ssid = "wyxbupt";
const char *password = "wuyuxin155";

std::vector<Peer> peers;

// MQTT设置
const char *mqtt_broker = "jfe2a84f.ala.cn-hangzhou.emqxsl.cn";
const char *mqtt_app_topic = "light/room2/app";
const char *mqtt_esp_topic = "light/room2/esp";
const char *mqtt_app_status_topic = "light/room2/app/status";
const char *mqtt_esp_status_topic = "light/room2/esp/status";
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

// 预设情景模式的RGB值
const int DEFAULT_SCENE_R = 0, DEFAULT_SCENE_G = 0, DEFAULT_SCENE_B = 0;
const int READING_SCENE_R = 0, READING_SCENE_G = 45, READING_SCENE_B = 215;
const int SLEEPING_SCENE_R = 204, SLEEPING_SCENE_G = 213, SLEEPING_SCENE_B = 247;
const int FILM_SCENE_R = 205, FILM_SCENE_G = 205, FILM_SCENE_B = 105;
const int PARTY_SCENE_R = 105, PARTY_SCENE_G = 180, PARTY_SCENE_B = 105;

// 颜色+亮度调节模式下各个颜色的RGB值
const int WHITE_MODE_R = 200, WHITE_MODE_G = 200, WHITE_MODE_B = 200;
const int WARM_MODE_R = 255, WARM_MODE_G = 210, WARM_MODE_B = 40;
const int NIGHT_MODE_R = 0, NIGHT_MODE_G = 180, NIGHT_MODE_B = 215;

// CA证书
const char ca_cert[] PROGMEM = R"EOF(
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
