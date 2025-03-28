#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <string.h>
#include <sstream>
#include <cstring>
#include <vector>
#include <ArduinoJson.h>

// WiFi settings
const char *ssid = "Divinity";
const char *password = "72698269426a";

// MQTT Broker settings
const char *mqtt_broker = "jfe2a84f.ala.cn-hangzhou.emqxsl.cn";
const char *mqtt_topic = "light/livingroom";
const char *mqtt_username = "Yunnai";
const char *mqtt_password = "azathoth";
const int mqtt_port = 8883;

const char *ntp_server = "pool.ntp.org";

const long gmt_offset_sec = 28800;
const int daylight_offset_sec = 0;

BearSSL::WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);

// CA certification
static const char ca_cert[]
PROGMEM = R"EOF(
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

void connectToMQTTBroker();

void syncTime();

void mqttCallback(char *topic, byte *payload, unsigned int length);

String ConverseDataToJson(const std::vector<std::pair<String, JsonVariant>>& data);

void sendMessage(String message);

void setup()
{
  Serial.begin(115200);
  connectToWiFi();
  syncTime();  // X.509 validation requires synchronization time
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();
}

// Try to connect to WiFi
void connectToWiFi()
{
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to the WiFi network");
}

void syncTime()
{
  configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
  Serial.print("Waiting for NTP time sync: ");
  // The time on the device will be abnormally early when it has not been synchronized
  while (time(nullptr) < 8 * 3600 * 2)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Time Synchronized");
  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
  {
    Serial.print("Current time: ");
    Serial.println(asctime(&timeinfo));
  }
  else
  {
    Serial.println("Failed to obtain local time.");
  }
}

// Try to connect to MQTT Broker
void connectToMQTTBroker()
{
  BearSSL::X509List serverTrustedCA(ca_cert);
  espClient.setTrustAnchors(&serverTrustedCA);
  while (!mqtt_client.connected())
  {
    String client_id = "esp8266-client-" + String(WiFi.macAddress());
    Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
    if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("Connected to MQTT broker");
      mqtt_client.subscribe(mqtt_topic);
      sendMessage(mqtt_topic, ConverseDataToJson("FirstKey", 12, "SecondKey", true));
    }
    else
    {
      char err_buf[128];
      espClient.getLastSSLError(err_buf, sizeof(err_buf));
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.println(mqtt_client.state());
      Serial.println("SSL error: ");
      Serial.println(err_buf);
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

// Handle received message
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message received on topic:");
  Serial.println(topic);
  Serial.print("Message:");
  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char) payload[i]);
  }
  Serial.println();
  Serial.println("--------------------------------");
}

void AddToDoc(StaticJsonDocument<512>& doc, const String& key, int value)
{
    doc[key] = value;
}

void AddToDoc(StaticJsonDocument<512>& doc, const String& key, bool value)
{
    doc[key] = value;
}

void AddToDoc(StaticJsonDocument<512>& doc, const String& key, const char* value)
{
    doc[key] = String(value);
}

template <typename T, typename... Args>
void AddToDocRecursive(StaticJsonDocument<512>& doc, const char* key, T value, Args... args) {
    AddToDoc(doc, key, value);
    AddToDocRecursive(doc, args...);
}

template <typename T>
void AddToDocRecursive(StaticJsonDocument<512>& doc, const char* key, T value) {
    AddToDoc(doc, key, value);
}


/*
  If you would like to converse data, please input data in pairs.
    String result = ConverseDataToJson("FirstKey", 12, "SecondKey", true);
*/
template<typename... Args>
String ConverseDataToJson(Args... args) {
    StaticJsonDocument<512> doc;

    static_assert(sizeof...(args) % 2 == 0, "Parameters must be passed in pairs.");

    AddToDocRecursive(doc, args...);
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

void sendMessage(const char *topic, const String& message)
{
  mqtt_client.publish(topic, message.c_str());
  Serial.println("Send message: " + message);
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (!mqtt_client.connected())
  {
    connectToMQTTBroker();
  }
  mqtt_client.loop();
}
