// config.h
#ifndef CONFIG_H
#define CONFIG_H

// WiFi设置
extern const char *ssid;
extern const char *password;

// MQTT设置
extern const char *mqtt_broker;
extern const char *mqtt_app_topic;  // 接收APP消息的主题
extern const char *mqtt_esp_topic;  // 发送ESP状态的主题
extern const char *mqtt_username;
extern const char *mqtt_password;
extern const int mqtt_port;

// NTP服务器设置
extern const char *ntp_server;
extern const long gmt_offset_sec; // GMT+8
extern const int daylight_offset_sec;

// 引脚定义 - ESP8266 NodeMCU引脚
extern const int photoSensorPin;
extern const int redPin;
extern const int greenPin;
extern const int bluePin;

// 预设情景模式的RGB值
extern const int READING_SCENE_R, READING_SCENE_G, READING_SCENE_B;
extern const int SLEEPING_SCENE_R, SLEEPING_SCENE_G, SLEEPING_SCENE_B;
extern const int FILM_SCENE_R, FILM_SCENE_G, FILM_SCENE_B;
extern const int PARTY_SCENE_R, PARTY_SCENE_G, PARTY_SCENE_B;
extern const int DEFAULT_SCENE_R, DEFAULT_SCENE_G, DEFAULT_SCENE_B;

// 颜色+亮度调节模式下各个颜色的RGB值
extern const int WHITE_MODE_R, WHITE_MODE_G, WHITE_MODE_B;
extern const int WARM_MODE_R, WARM_MODE_G, WARM_MODE_B;
extern const int NIGHT_MODE_R, NIGHT_MODE_G, NIGHT_MODE_B;


extern const char ca_cert[]; 

#endif
