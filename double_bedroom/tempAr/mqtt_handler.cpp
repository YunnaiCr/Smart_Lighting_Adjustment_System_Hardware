#include "mqtt_handler.h"

// 发送模式变更通知到APP（通过普通通道发送JSON）
void sendModeChangeToApp(const String& newMode) {
  if (!mqtt_client.connected()) {
    Serial.println("MQTT未连接，无法发送模式变更通知");
    return;
  }
  String jsonCmd = ConverseDataToJson(
    "operationMode", newMode
  );
  if (mqtt_client.publish(mqtt_app_topic, jsonCmd.c_str())) {
    Serial.println("已发送模式变更通知: " + jsonCmd);
  } else {
    Serial.println("模式变更通知发送失败!");
  }
}

// 从语音模式切换到其他模式
void switchFromVoiceMode(const String& newMode) {
  setOperationMode(newMode);
  delay(100);
  sendModeChangeToApp(newMode);//esp端用语音切换模式和app进行同步
}

void sendStatusToApp() {
  if (!mqtt_client.connected()) {
    Serial.println("MQTT未连接，无法发送状态");
    return;
  }
  
  // 构建JSON状态（移除fullStatus参数，只发送实际状态值）
  String statusJson = ConverseDataToJson(
    "operationMode", operationMode,
    "part", part,
    "sceneMode", sceneMode,
    "color", currentBaseColorName,
    "brightness", currentBrightnessLevel
  );
  
  if (mqtt_client.publish(mqtt_app_topic, statusJson.c_str())) {
    Serial.println("已发送状态: " + statusJson);
  } else {
    Serial.println("状态发送失败!");
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
  
  String topicStr = String(topic);
  
  // 处理状态通道的同步请求
  if (topicStr == String(mqtt_esp_status_topic)) {
    // 检查是否是同步请求
    if (message.indexOf("sync") != -1 || message.equals("sync")) {
      Serial.println("收到APP同步请求，发送当前状态");
      sendStatusToApp();
      return;
    }
    Serial.println("收到未知状态通道消息: " + message);
    return;
  }
  
  // 处理普通通道的JSON消息
  if (topicStr == String(mqtt_esp_topic)) {
    // 解析JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
      Serial.print(F("JSON解析失败: "));
      Serial.println(error.f_str());
      return;
    }
    
    // 统一处理所有参数 - 先保存，最后应用
    bool needUpdate = false;
    
    // 保存part
    if (doc.containsKey("part")) {
      bool newPart = doc["part"].as<bool>();
      if (newPart == false || newPart == true) {
        if (part != newPart) {
          part = newPart;
          needUpdate = true;
          Serial.println("更新 part = " + String(part));
        }
      } else {
        Serial.println("接收到无效 part 值: " + String(newPart));
      }
    }
    
    // 保存sceneMode
    if (doc.containsKey("sceneMode")) {
      String newSceneMode = doc["sceneMode"].as<String>();
      if (sceneMode != newSceneMode) {
        sceneMode = newSceneMode;
        needUpdate = true;
        Serial.println("更新 sceneMode = " + sceneMode);
      }
    }
    
    // 保存color
    if (doc.containsKey("color")) {
      String newColor = doc["color"].as<String>();
      if (currentBaseColorName != newColor) {
        if (newColor == "white") {
          currentColorBaseR = WHITE_MODE_R;
          currentColorBaseG = WHITE_MODE_G;
          currentColorBaseB = WHITE_MODE_B;
          currentBaseColorName = "white";
        } else if (newColor == "warm") {
          currentColorBaseR = WARM_MODE_R;
          currentColorBaseG = WARM_MODE_G;
          currentColorBaseB = WARM_MODE_B;
          currentBaseColorName = "warm";
        } else if (newColor == "night") {
          currentColorBaseR = NIGHT_MODE_R;
          currentColorBaseG = NIGHT_MODE_G;
          currentColorBaseB = NIGHT_MODE_B;
          currentBaseColorName = "night";
        }
        needUpdate = true;
        Serial.println("更新 color = " + currentBaseColorName);
      }
    }
    
    // 保存brightness
    if (doc.containsKey("brightness")) {
      float newBrightness = doc["brightness"].as<float>();
      newBrightness = constrain(newBrightness, 1.0f, 5.0f);
      if (abs(currentBrightnessLevel - newBrightness) > 0.01) {
        currentBrightnessLevel = newBrightness;
        needUpdate = true;
        Serial.println("更新 brightness = " + String(currentBrightnessLevel));
      }
    }
    
    // 处理RGB直接控制
    if (doc.containsKey("Red") || doc.containsKey("Green") || doc.containsKey("Blue")) {
      bool colorChanged = false;
      
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
      
      if (colorChanged && operationMode == "manualMode" && part == true) {
        setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
        return; // RGB直接控制优先，无需处理其他参数
      }
    }
    
    // 最后处理operationMode，这会实际应用设置
    if (doc.containsKey("operationMode")) {
      String newMode = doc["operationMode"].as<String>();
      if (operationMode != newMode) {
        Serial.println("设置操作模式为: " + newMode);
        setOperationMode(newMode);
      } else if (needUpdate && operationMode == "manualMode") {
        // 如果没有模式变化但有其他参数变化，且在手动模式，应用更新
        if (part == false) {
          setSceneMode(sceneMode);
        } else {
          adjustBrightnessForManualMode(currentBrightnessLevel);
        }
      }
    } else if (needUpdate && operationMode == "manualMode") {
      // 如果没有指定模式但有参数变化，且在手动模式，应用更新
      if (part == false) {
        setSceneMode(sceneMode);
      } else {
        adjustBrightnessForManualMode(currentBrightnessLevel);
      }
    }
  }
}
