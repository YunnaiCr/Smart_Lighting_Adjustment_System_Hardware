// light_control.cpp
#include "light_control.h"

// LED颜色设置
void setLEDColor(int red, int green, int blue) {
  int espRed = map(red, 0, 255, 0, 255);   
  int espGreen = map(green, 0, 255, 0, 255);
  int espBlue = map(blue, 0, 255, 0, 255);

  analogWrite(redPin, espRed);
  analogWrite(greenPin, espGreen);
  analogWrite(bluePin, espBlue);
}

// 情景模式（手动模式 part == false）
void setSceneMode(const String& mode) {
  String oldSceneMode = sceneMode;
  bool applied = false;
  
  if (mode == "Reading") {
    manualRedValue = READING_SCENE_R;
    manualGreenValue = READING_SCENE_G;
    manualBlueValue = READING_SCENE_B;
    applied = true;
  } else if (mode == "Sleeping") {
    manualRedValue = SLEEPING_SCENE_R;
    manualGreenValue = SLEEPING_SCENE_G;
    manualBlueValue = SLEEPING_SCENE_B;
    applied = true;
  } else if (mode == "Film") {
    manualRedValue = FILM_SCENE_R;
    manualGreenValue = FILM_SCENE_G;
    manualBlueValue = FILM_SCENE_B;
    applied = true;
  } else if (mode == "Party") {
    manualRedValue = PARTY_SCENE_R;
    manualGreenValue = PARTY_SCENE_G;
    manualBlueValue = PARTY_SCENE_B;
    applied = true;
  } else if (mode == "Default") {
    manualRedValue = DEFAULT_SCENE_R;
    manualGreenValue = DEFAULT_SCENE_G;
    manualBlueValue = DEFAULT_SCENE_B;
    applied = true;
  } else {
    Serial.println("未知的预设情景模式: " + mode);
    return;
  }
  
  if (applied) {
    setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
    Serial.println("已设置情景模式: " + mode);
  }
}

// 颜色+亮度模式（手动模式 part == 1）
// 设置颜色基准
void setBaseColorForManualMode(const String& colorName) {
  if (colorName == "white") {
    currentColorBaseR = WHITE_MODE_R;
    currentColorBaseG = WHITE_MODE_G;
    currentColorBaseB = WHITE_MODE_B;
    currentBaseColorName = "white";
  } else if (colorName == "warm") {
    currentColorBaseR = WARM_MODE_R;
    currentColorBaseG = WARM_MODE_G;
    currentColorBaseB = WARM_MODE_B;
    currentBaseColorName = "warm";
  } else if (colorName == "night") {
    currentColorBaseR = NIGHT_MODE_R;
    currentColorBaseG = NIGHT_MODE_G;
    currentColorBaseB = NIGHT_MODE_B;
    currentBaseColorName = "night";
  } else {
    Serial.println("未知颜色: " + colorName);
    return;
  }
  
  adjustBrightnessForManualMode(currentBrightnessLevel); // 重新应用亮度
  Serial.println("手动模式颜色基准设置为: " + colorName);
}

// 调整亮度
void adjustBrightnessForManualMode(float level) {
  // 1. 限制亮度等级到 [1.0, 5.0]
  currentBrightnessLevel = constrain(level, 1.0f, 5.0f);
  
  // scale ∈ [0.2, 1.0]
  float scale = currentBrightnessLevel / 5.0f;  
  
  // 先算出按比例缩放后的"亮度分量"
  int scaledR = int(currentColorBaseR * scale);
  int scaledG = int(currentColorBaseG * scale);
  int scaledB = int(currentColorBaseB * scale);
  
  // 再反转到共阳极的 PWM 值域（0→全亮, 255→全灭）
  manualRedValue = constrain(255 - scaledR, 0, 255);
  manualGreenValue = constrain(255 - scaledG, 0, 255);
  manualBlueValue = constrain(255 - scaledB, 0, 255);
  
  setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
  Serial.printf("亮度等级: %.1f (比例: %.2f), PWM: (%d, %d, %d)\n",
                currentBrightnessLevel, scale,
                manualRedValue, manualGreenValue, manualBlueValue);
}

// 处理part切换逻辑
void handlePartChange(bool newPart) {
  if (part == newPart) return;
  
  part = newPart;
  Serial.print("已切换 manualMode 子模式为: ");
  Serial.println(part == false ? "sceneMode (0)" : "colorBrightness (1)");
  
  if (part == false) {
    // 切换到情景模式
    setSceneMode(sceneMode);
  } else {
    adjustBrightnessForManualMode(currentBrightnessLevel);
  }
}

// 基于光敏传感器调整灯光
void adjustLightingBasedOnSensor() {
  int sensorValue = analogRead(photoSensorPin);
  
  // 添加阈值判断：当光敏值超过90%时关灯
  const int LIGHT_THRESHOLD = 100;  // 10% of 1023
  
  if (sensorValue <= LIGHT_THRESHOLD) {
    // 环境光过强，关闭灯光(共阳极下设为255)
    setLEDColor(255, 255, 255);
    Serial.println("环境光过强(>90%)，灯光已关闭");
    return;
  }
  
  // 正常情况下按亮度比例调节
  int pwmValue = map(sensorValue, 0, 1023, 255, 0);
  pwmValue = constrain(pwmValue, 0, 255);
  setLEDColor(pwmValue, pwmValue, pwmValue);
}

void checkVoiceCommands() {
      if (operationMode != "voiceMode") {
        return;
      }
      if (Serial.available() >= 4) {
        byte receivedData[4];
        Serial.readBytes(receivedData, 4);
        String oldVoiceCmd = voiceCommand;
        String commandHex = "";
        for (int i = 0; i < 4; i++) {
          char hexStr[3];
          sprintf(hexStr, "%02X", receivedData[i]);
          commandHex += hexStr;
        }
        Serial.print("收到语音指令(HEX): ");
        Serial.println(commandHex);
        bool commandProcessed = false;
        
        // 处理模式切换命令
        if (receivedData[0] == 0xD1 && receivedData[1] == 0xD1 &&
            receivedData[2] == 0xD1 && receivedData[3] == 0xD1) {
          Serial.println("语音命令:卧室自动模式");
          switchFromVoiceMode("autoMode");
          return; // 直接返回,因为已经不在语音模式了
        }
        else if (receivedData[0] == 0xD2 && receivedData[1] == 0xD2 &&
                 receivedData[2] == 0xD2 && receivedData[3] == 0xD2) {
          Serial.println("语音命令:卧室手动模式");
          switchFromVoiceMode("manualMode");
          return; // 直接返回,因为已经不在语音模式了
        }
        
        // 处理本地房间命令 - 直接控制灯光
        else if (receivedData[0] == 0xA1 && receivedData[1] == 0xA1 &&
                 receivedData[2] == 0xA1 && receivedData[3] == 0xA1) { // 打开卧室灯
          voiceReddata = WHITE_MODE_R;
          voiceGreendata = WHITE_MODE_G;
          voiceBluedata = WHITE_MODE_B;
          voiceCommand = "White";
          Serial.println("语音命令:打开卧室灯");
          commandProcessed = true;
        }
        else if (receivedData[0] == 0xA2 && receivedData[1] == 0xA2 &&
                 receivedData[2] == 0xA2 && receivedData[3] == 0xA2) { // 关闭卧室灯
          voiceReddata = 255;
          voiceGreendata = 255;
          voiceBluedata = 255;
          voiceCommand = "Black";
          Serial.println("语音命令:关闭卧室灯");
          commandProcessed = true;
        }
        else if (receivedData[0] == 0xA3 && receivedData[1] == 0xA3 &&
                 receivedData[2] == 0xA3 && receivedData[3] == 0xA3) { // 卧室灯调亮
          if(voiceReddata == voiceGreendata && voiceBluedata == voiceGreendata) { // 灰度/白光
            int currentBrightness = voiceReddata;
            currentBrightness = max(0, currentBrightness - 40);
            voiceReddata = voiceGreendata = voiceBluedata = currentBrightness;
          } else { // 彩色光
            voiceReddata = max(0, (int)(voiceReddata * 0.8));
            voiceGreendata = max(0, (int)(voiceGreendata * 0.8));
            voiceBluedata = max(0, (int)(voiceBluedata * 0.8));
          }
          voiceCommand = "light up";
          Serial.println("语音命令:卧室灯调亮");
          commandProcessed = true;
        }
        else if (receivedData[0] == 0xA4 && receivedData[1] == 0xA4 &&
                 receivedData[2] == 0xA4 && receivedData[3] == 0xA4) { // 卧室灯调暗
          if(voiceReddata == voiceGreendata && voiceBluedata == voiceGreendata) { // 灰度/白光
            int currentBrightness = voiceReddata;
            currentBrightness = min(255, currentBrightness + 40);
            voiceReddata = voiceGreendata = voiceBluedata = currentBrightness;
          } else { // 彩色光
            voiceReddata = min(255, (int)(voiceReddata / 0.8));
            voiceGreendata = min(255, (int)(voiceGreendata / 0.8));
            voiceBluedata = min(255, (int)(voiceBluedata / 0.8));
          }
          voiceCommand = "light down";
          Serial.println("语音命令:卧室灯调暗");
          commandProcessed = true;
        }
        else if (receivedData[0] == 0xA6 && receivedData[1] == 0xA6 &&
                 receivedData[2] == 0xA6 && receivedData[3] == 0xA6) { // 卧室阅读模式
          voiceReddata = READING_SCENE_R;
          voiceGreendata = READING_SCENE_G;
          voiceBluedata = READING_SCENE_B;
          voiceCommand = "Read model";
          Serial.println("语音命令:卧室阅读模式");
          commandProcessed = true;
        }
        else if (receivedData[0] == 0xA7 && receivedData[1] == 0xA7 &&
                 receivedData[2] == 0xA7 && receivedData[3] == 0xA7) { // 关闭卧室阅读模式
          voiceReddata = 255;
          voiceGreendata = 255;
          voiceBluedata = 255;
          voiceCommand = "Read model-c";
          Serial.println("语音命令:关闭卧室阅读模式");
          commandProcessed = true;
        }
        else if (receivedData[0] == 0xA8 && receivedData[1] == 0xA8 &&
                 receivedData[2] == 0xA8 && receivedData[3] == 0xA8) { // 打开卧室睡眠模式
          voiceReddata = SLEEPING_SCENE_R;
          voiceGreendata = SLEEPING_SCENE_G;
          voiceBluedata = SLEEPING_SCENE_B;
          voiceCommand = "Sleep model";
          Serial.println("语音命令:打开卧室睡眠模式");
          commandProcessed = true;
        }
        else if (receivedData[0] == 0xA9 && receivedData[1] == 0xA9 &&
                 receivedData[2] == 0xA9 && receivedData[3] == 0xA9) { // 关闭卧室睡眠模式
          voiceReddata = 255;
          voiceGreendata = 255;
          voiceBluedata = 255;
          voiceCommand = "Sleep model-c";
          Serial.println("语音命令:关闭卧室睡眠模式");
          commandProcessed = true;
        }
        
        // 处理跨房间命令 - 发送命令到客厅ESP
        else if (receivedData[0] == 0xE1 && receivedData[1] == 0xE1 &&
                 receivedData[2] == 0xE1 && receivedData[3] == 0xE1) { // 打开客厅灯
          // 构建控制命令
          StaticJsonDocument<128> doc;
          doc["cmd"] = "light_on";
          String jsonCmd;
          serializeJson(doc, jsonCmd);
          
          // 发送到客厅ESP
          sendToPeer("ESP_LIVING_ROOM", jsonCmd);
          Serial.println("语音命令:打开客厅灯(跨房间)");
          commandProcessed = true;
        }
        else if (receivedData[0] == 0xE2 && receivedData[1] == 0xE2 &&
                 receivedData[2] == 0xE2 && receivedData[3] == 0xE2) { // 关闭客厅灯
          StaticJsonDocument<128> doc;
          doc["cmd"] = "light_off";
          String jsonCmd;
          serializeJson(doc, jsonCmd);
          
          sendToPeer("ESP_LIVING_ROOM", jsonCmd);
          Serial.println("语音命令:关闭客厅灯(跨房间)");
          commandProcessed = true;
        }
        else if (receivedData[0] == 0xE3 && receivedData[1] == 0xE3 &&
                 receivedData[2] == 0xE3 && receivedData[3] == 0xE3) { // 客厅灯调亮
          StaticJsonDocument<128> doc;
          doc["cmd"] = "brightness_up";
          String jsonCmd;
          serializeJson(doc, jsonCmd);
          
          sendToPeer("ESP_LIVING_ROOM", jsonCmd);
          Serial.println("语音命令:客厅灯调亮(跨房间)");
          commandProcessed = true;
        }
        else if (receivedData[0] == 0xE4 && receivedData[1] == 0xE4 &&
                 receivedData[2] == 0xE4 && receivedData[3] == 0xE4) { // 客厅灯调暗
          StaticJsonDocument<128> doc;
          doc["cmd"] = "brightness_down";
          String jsonCmd;
          serializeJson(doc, jsonCmd);
          
          sendToPeer("ESP_LIVING_ROOM", jsonCmd);
          Serial.println("语音命令:客厅灯调暗(跨房间)");
          commandProcessed = true;
        }
        else if (receivedData[0] == 0xE5 && receivedData[1] == 0xE5 &&
                 receivedData[2] == 0xE5 && receivedData[3] == 0xE5) { // 客厅影视模式
          StaticJsonDocument<128> doc;
          doc["cmd"] = "scene_film";
          String jsonCmd;
          serializeJson(doc, jsonCmd);
          
          sendToPeer("ESP_LIVING_ROOM", jsonCmd);
          Serial.println("语音命令:客厅影视模式(跨房间)");
          commandProcessed = true;
        }
        else if (receivedData[0] == 0xE7 && receivedData[1] == 0xE7 &&
                 receivedData[2] == 0xE7 && receivedData[3] == 0xE7) { // 客厅派对模式
          StaticJsonDocument<128> doc;
          doc["cmd"] = "scene_party";
          String jsonCmd;
          serializeJson(doc, jsonCmd);
          
          sendToPeer("ESP_LIVING_ROOM", jsonCmd);
          Serial.println("语音命令:客厅派对模式(跨房间)");
          commandProcessed = true;
        }
        
        // 应用灯光变化
        if (commandProcessed) {
          setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
        }
      }
    }
  
// 设置操作模式
void setOperationMode(const String& newMode) {
  String oldOperationMode = operationMode;
  if (oldOperationMode == newMode) {
    return;
  }
  
  if (oldOperationMode == "voiceMode" && newMode != "voiceMode") {
    byte exitVoiceCmd[] = {0xAA, 0x01, 0x01, 0x00, 0x00, 0x00, 0x55, 0xAA};
    Serial.write(exitVoiceCmd, sizeof(exitVoiceCmd));
    Serial.println("串口发送：退出语音模式指令");
  }
  else if (oldOperationMode != "voiceMode" && newMode == "voiceMode") {
    byte enterVoiceCmd[] = {0xAA, 0x02, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA};
    Serial.write(enterVoiceCmd, sizeof(enterVoiceCmd));
    Serial.println("串口发送：进入语音模式指令");
  }
  
  operationMode = newMode;
  
  if (newMode == "autoMode") {
    Serial.println("已切换到自动调节模式");
  }
  else if (newMode == "manualMode") {
    if (!manualModeEverEntered) {
      part = false;
      sceneMode = "Reading";
      setManualModeState(part, sceneMode, READING_SCENE_R, READING_SCENE_G, READING_SCENE_B);
      manualModeEverEntered = true;
    } else {
      if (part == false) {
        setSceneMode(sceneMode);
      } else {
        setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
      }
    }
    Serial.println("已切换到手动控制模式");
  }
  else if (newMode == "voiceMode") {
    voiceCommand = "None";
    Serial.println("已切换到语音控制模式");
  }
  else {
    Serial.println("设置了未知的操作模式: " + newMode);
    operationMode = oldOperationMode;
    return;
  }
}

// setManualModeState 函数保持不变，它负责应用传入的特定手动模式状态
void setManualModeState(bool targetPart, String targetSceneMode, int r, int g, int b) {
  part = targetPart;
  sceneMode = targetSceneMode;
  manualRedValue = r;
  manualGreenValue = g;
  manualBlueValue = b;
  if (part == false) {
    setSceneMode(sceneMode);
  } else {
    setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
  }
}

// Party渐变函数
void updatePartyLights() {
  // 每20ms更新一次，产生平滑变化
  unsigned long currentMillis = millis();
  if (currentMillis - lastPartyUpdate < 10) return;
  lastPartyUpdate = currentMillis;
  
  // 更新色相值（彩虹循环）
  partyHue = (partyHue + 1) % 360;
  
  // 将HSV转换为RGB
  int r, g, b;
  hsvToRgb(partyHue, 100, 100, r, g, b); // 饱和度和亮度最大
  
  // 为共阳极LED反转值
  manualRedValue = 255 - r;
  manualGreenValue = 255 - g;
  manualBlueValue = 255 - b;
  
  setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
}

//party频闪函数
void updateStrobeEffect() {
  unsigned long currentMillis = millis();
  
  // 根据设定的速度控制更新频率
  if (currentMillis - lastStrobeUpdate < strobeSpeed) {
    return;
  }
  
  lastStrobeUpdate = currentMillis;
  
  // 切换开关状态
  strobeState = !strobeState;
  
  if (strobeState) {
    // 灯亮状态
    switch (strobeColor) {
      case 0: // 白光模式
        // 共阳极LED: 0,0,0 是最亮的白光
        manualRedValue = 0;
        manualGreenValue = 0;
        manualBlueValue = 0;
        break;
        
      case 1: // 彩色模式 - 每次闪烁使用不同颜色
        {
          // 每次闪烁时改变色相
          strobeColorHue = (strobeColorHue + 30) % 360; // 每次增加30度
          
          int r, g, b;
          hsvToRgb(strobeColorHue, 100, 100, r, g, b);
          
          // 为共阳极LED反转值
          manualRedValue = 255 - r;
          manualGreenValue = 255 - g;
          manualBlueValue = 255 - b;
        }
        break;
        
      case 2: // 渐变色模式 - 慢慢变化颜色
        {
          // 缓慢改变色相
          strobeColorHue = (strobeColorHue + 3) % 360; // 每次增加3度
          
          int r, g, b;
          hsvToRgb(strobeColorHue, 100, 100, r, g, b);
          
          // 为共阳极LED反转值
          manualRedValue = 255 - r;
          manualGreenValue = 255 - g;
          manualBlueValue = 255 - b;
        }
        break;
    }
  } else {
    // 灯灭状态 - 共阳极LED: 255,255,255 是完全关闭
    manualRedValue = 255;
    manualGreenValue = 255;
    manualBlueValue = 255;
  }
  
  // 应用LED设置
  setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
}

// 处理从其他ESP接收到的命令
void handleRemoteCommand(const String& message) {
  // 只有在语音模式下才处理跨房间命令
  if (operationMode != "voiceMode") {
    Serial.println("非语音模式，忽略远程命令");
    return;
  }

  // 解析JSON命令
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print("解析JSON失败: ");
    Serial.println(error.c_str());
    return;
  }
  
  if (!doc.containsKey("cmd")) {
    Serial.println("无效的远程命令格式");
    return;
  }
  
  String cmd = doc["cmd"].as<String>();
  Serial.print("处理远程命令: ");
  Serial.println(cmd);
  
  // 执行相应命令
  if (cmd == "light_on") {
    // 打开灯
    voiceReddata = WHITE_MODE_R;
    voiceGreendata = WHITE_MODE_G;
    voiceBluedata = WHITE_MODE_B;
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  } 
  else if (cmd == "light_off") {
    // 关闭灯
    voiceReddata = 255;
    voiceGreendata = 255;
    voiceBluedata = 255;
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  }
  else if (cmd == "brightness_up") {
    // 调亮
    if(voiceReddata == voiceGreendata && voiceBluedata == voiceGreendata) {
      int currentBrightness = voiceReddata;
      currentBrightness = max(0, currentBrightness - 40);
      voiceReddata = voiceGreendata = voiceBluedata = currentBrightness;
    } else {
      voiceReddata = max(0, (int)(voiceReddata * 0.8));
      voiceGreendata = max(0, (int)(voiceGreendata * 0.8));
      voiceBluedata = max(0, (int)(voiceBluedata * 0.8));
    }
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  }
  else if (cmd == "brightness_down") {
    // 调暗
    if(voiceReddata == voiceGreendata && voiceBluedata == voiceGreendata) {
      int currentBrightness = voiceReddata;
      currentBrightness = min(255, currentBrightness + 40);
      voiceReddata = voiceGreendata = voiceBluedata = currentBrightness;
    } else {
      voiceReddata = min(255, (int)(voiceReddata / 0.8));
      voiceGreendata = min(255, (int)(voiceGreendata / 0.8));
      voiceBluedata = min(255, (int)(voiceBluedata / 0.8));
    }
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  }
  else if (cmd == "scene_film") {
    // 影视模式
    voiceReddata = FILM_SCENE_R;
    voiceGreendata = FILM_SCENE_G;
    voiceBluedata = FILM_SCENE_B;
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  }
  else if (cmd == "scene_party") {
    // 派对模式
    voiceReddata = PARTY_SCENE_R;
    voiceGreendata = PARTY_SCENE_G;
    voiceBluedata = PARTY_SCENE_B;
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  }
  else if (cmd == "scene_reading") {
    // 阅读模式
    voiceReddata = READING_SCENE_R;
    voiceGreendata = READING_SCENE_G;
    voiceBluedata = READING_SCENE_B;
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  }
  else if (cmd == "scene_sleep") {
    // 睡眠模式
    voiceReddata = SLEEPING_SCENE_R;
    voiceGreendata = SLEEPING_SCENE_G;
    voiceBluedata = SLEEPING_SCENE_B;
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  }
}

// 将HSV转换为RGB的辅助函数
void hsvToRgb(int h, int s, int v, int &r, int &g, int &b) {
  // h: 0-360, s: 0-100, v: 0-100
  s = s * 255 / 100;
  v = v * 255 / 100;
  
  int region = h / 60;
  int remainder = (h - (region * 60)) * 255 / 60;
  
  int p = (v * (255 - s)) >> 8;
  int q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  int t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
  
  switch (region) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }
}
