#include "mqtt_handler.h"

// 发送模式变更通知到APP
void sendModeChangeToApp(const String& newMode) {
  if (!mqtt_client.connected()) {
    Serial.println("MQTT未连接，无法发送模式变更通知");
    return;
  }
  String jsonCmd = ConverseDataToJson(
    "operationMode", newMode
  );
  if (mqtt_client.publish(mqtt_esp_topic, jsonCmd.c_str())) {
    Serial.println("已发送模式变更通知: " + jsonCmd);
  } else {
    Serial.println("模式变更通知发送失败!");
  }
}

// 从语音模式切换到其他模式
void switchFromVoiceMode(const String& newMode) {
  setOperationMode(newMode);
  sendModeChangeToApp(newMode);//esp端用语音切换模式和app进行同步
}

// 状态字符串解析函数
void parseStatusString(const String& statusStr) {
  int index = 0;
  int lastIndex = 0;
  String values[5]; // 五个值: operationMode, part, sceneMode, color, brightness
  
  // 分割字符串
  for(int i = 0; i < 5; i++) {
    index = statusStr.indexOf(',', lastIndex);
    if(index == -1 && i < 4) {
      Serial.println("状态字符串格式错误: " + statusStr);
      return;
    }
    if(i == 4 || index == -1) {
      values[i] = statusStr.substring(lastIndex);
    } else {
      values[i] = statusStr.substring(lastIndex, index);
      lastIndex = index + 1;
    }
  }
  
  Serial.println("解析状态: [操作模式=" + values[0] + ", part=" + values[1] +
                ", 场景=" + values[2] + ", 颜色=" + values[3] +
                ", 亮度=" + values[4] + "]");
  
  // 保存所有参数
  String newMode = values[0].length() > 0 ? values[0] : operationMode;
  int newPart = values[1].length() > 0 ? values[1].toInt() : part;
  String newSceneMode = values[2].length() > 0 ? values[2] : sceneMode;
  
  // 处理颜色更新
  if(values[3].length() > 0) {
    String newColor = values[3];
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
    Serial.println("更新 color = " + currentBaseColorName);
  }
  
  // 处理亮度更新
  if(values[4].length() > 0) {
    currentBrightnessLevel = values[4].toFloat();
    currentBrightnessLevel = constrain(currentBrightnessLevel, 1.0f, 5.0f);
    Serial.println("更新 brightness = " + String(currentBrightnessLevel));
  }
  
  // 根据颜色和亮度计算RGB值
  if(values[3].length() > 0 || values[4].length() > 0) {
    float scale = currentBrightnessLevel / 5.0f;
    int scaledR = int(currentColorBaseR * scale);
    int scaledG = int(currentColorBaseG * scale);
    int scaledB = int(currentColorBaseB * scale);
    
    manualRedValue = constrain(255 - scaledR, 0, 255);
    manualGreenValue = constrain(255 - scaledG, 0, 255);
    manualBlueValue = constrain(255 - scaledB, 0, 255);
    Serial.printf("根据颜色和亮度更新RGB: (%d, %d, %d)\n", manualRedValue, manualGreenValue, manualBlueValue);
  }
  
  // 更新部分和场景模式
  part = newPart;
  sceneMode = newSceneMode;
  
  // 设置操作模式，应用设置
  if(newMode != operationMode) {
    setOperationMode(newMode);
  } else if(newMode == "manualMode") {
    // 已经在手动模式，直接应用设置
    if(part == 0) {
      setSceneMode(sceneMode);
    } else {
      setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
    }
  }
  
  Serial.println("状态已完全应用");
}

// 向app发送状态字符串
void sendStatusString() {
  if (!mqtt_client.connected()) {
    Serial.println("MQTT未连接，无法发送状态");
    return;
  }
  
  // 构建状态字符串：operationMode,part,sceneMode,color,brightness
  String statusStr = operationMode + "," +
                     String(part) + "," +
                     sceneMode + "," +
                     currentBaseColorName + "," +
                     String(currentBrightnessLevel);
  
  if (mqtt_client.publish(mqtt_esp_topic, statusStr.c_str())) {
    Serial.println("已发送状态: " + statusStr);
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
  
  // 如果是APP主题的消息
  if (String(topic) == String(mqtt_app_topic)) {
    
    // 检查是否是同步请求
   if (message.indexOf("sync") != -1 || message.equals("sync")) {
      Serial.println("收到APP同步请求，发送当前状态");
        sendStatusString();
       return;
    }
    
    // 尝试解析为JSON (普通控制命令)
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
      // 如果不是JSON，尝试作为状态字符串解析
      if (message.indexOf(',') != -1) {
        Serial.println("接收到状态字符串，解析中...");
        parseStatusString(message);
      } else {
        Serial.print(F("消息格式无法识别: "));
        Serial.println(message);
      }
      return;
    }
    
    // 处理JSON命令
    if (doc.containsKey("operationMode")) {
      String newOpModeFromMqtt = doc["operationMode"].as<String>();
      if (operationMode != newOpModeFromMqtt) {
        setOperationMode(newOpModeFromMqtt);
      }
    }
    
    if (operationMode == "manualMode" && doc.containsKey("part")) {
      int newPart = doc["part"].as<int>();
      if (newPart == 0 || newPart == 1) {
        handlePartChange(newPart); // 使用专门的处理函数
      } else {
        Serial.println("接收到无效 part 值: " + String(newPart));
      }
    }
    
    if (operationMode == "manualMode") {
      if (part == 0) { // 处理情景模式 (sceneMode) 命令
        if (doc.containsKey("sceneMode")) {
          String newSceneMode = doc["sceneMode"].as<String>();
          if (sceneMode != newSceneMode) {
            setSceneMode(newSceneMode);
            sceneMode = newSceneMode;
          }
        }
      } else { // part == 1: 处理颜色(color) + 亮度(brightness) 命令
        if (doc.containsKey("color")) {
          String newColor = doc["color"].as<String>();
          setBaseColorForManualMode(newColor);
        }
        if (doc.containsKey("brightness")) {
          float newBrightness = doc["brightness"].as<float>();
          adjustBrightnessForManualMode(newBrightness);
        }
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
          if (colorChanged) {
            setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
          }
        }
      }
    }
  }
}
