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

// 情景模式（手动模式 part == 0）
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
    applied = true;}
    else {
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

// 先算出按比例缩放后的“亮度分量”
int scaledR = int(currentColorBaseR * scale);
int scaledG = int(currentColorBaseG * scale);
int scaledB = int(currentColorBaseB * scale);

// 再反转到共阳极的 PWM 值域（0→全亮, 255→全灭）
manualRedValue   = constrain(255 - scaledR, 0, 255);
manualGreenValue = constrain(255 - scaledG, 0, 255);
manualBlueValue  = constrain(255 - scaledB, 0, 255);

setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
Serial.printf("亮度等级: %.1f (比例: %.2f), PWM: (%d, %d, %d)\n",
              currentBrightnessLevel, scale,
              manualRedValue, manualGreenValue, manualBlueValue);
}

// 处理part切换逻辑
void handlePartChange(int newPart) {
  if (part == newPart) return;
  
  part = newPart;
  Serial.print("已切换 manualMode 子模式为: ");
  Serial.println(part == 0 ? "sceneMode (0)" : "colorBrightness (1)");
  
  if (part == 0) {
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

// 检查语音命令
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
    
    if (receivedData[0] == 0xA3 && receivedData[1] == 0xA3 &&
        receivedData[2] == 0xA3 && receivedData[3] == 0xA3) { // 开启客厅灯 (白光)
      voiceReddata = WHITE_MODE_R;
      voiceGreendata = WHITE_MODE_G;
      voiceBluedata = WHITE_MODE_B;
      voiceCommand = "White";
      Serial.println("语音命令：开启客厅灯");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0xA4 && receivedData[1] == 0xA4 &&
             receivedData[2] == 0xA4 && receivedData[3] == 0xA4) { // 关闭客厅灯
      voiceReddata = 255; // 假设255,255,255是灯灭 (共阳极)
      voiceGreendata = 255;
      voiceBluedata = 255;
      voiceCommand = "Black";
      Serial.println("语音命令:关闭客厅灯");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0xA7 && receivedData[1] == 0xA7 &&
             receivedData[2] == 0xA7 && receivedData[3] == 0xA7) { // 亮度调高
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
      Serial.println("语音命令：亮度调高");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0xA8 && receivedData[1] == 0xA8 &&
             receivedData[2] == 0xA8 && receivedData[3] == 0xA8) { // 亮度调低
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
      Serial.println("语音命令：亮度调低");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0xA9 && receivedData[1] == 0xA9 &&
             receivedData[2] == 0xA9 && receivedData[3] == 0xA9) { // 打开柔光灯 (暖光)
      voiceReddata = WARM_MODE_R;
      voiceGreendata = WARM_MODE_G;
      voiceBluedata = WARM_MODE_B;
      voiceCommand = "light Soft";
      Serial.println("语音命令：打开柔光灯");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0x10 && receivedData[1] == 0x10 &&
             receivedData[2] == 0x10 && receivedData[3] == 0x10) { // 阅读模式开启 (暖光)
      voiceReddata = READING_SCENE_R;
      voiceGreendata = READING_SCENE_G;
      voiceBluedata = READING_SCENE_B;
      voiceCommand = "Read model";
      Serial.println("语音命令：打开阅读模式");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0x11 && receivedData[1] == 0x11 &&
             receivedData[2] == 0x11 && receivedData[3] == 0x11) { // 阅读模式关闭 (灯灭)
      voiceReddata = 255;
      voiceGreendata = 255;
      voiceBluedata = 255;
      voiceCommand = "Read model-c";
      Serial.println("语音命令：关闭阅读模式");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0x12 && receivedData[1] == 0x12 &&
             receivedData[2] == 0x12 && receivedData[3] == 0x12) { // 睡眠模式开启
      voiceReddata = SLEEPING_SCENE_R;
      voiceGreendata = SLEEPING_SCENE_G;
      voiceBluedata = SLEEPING_SCENE_B;
      voiceCommand = "Sleep model";
      Serial.println("语音命令：打开睡眠模式");
      commandProcessed = true;
    }
    else if (receivedData[0] == 0x13 && receivedData[1] == 0x13 &&
             receivedData[2] == 0x13 && receivedData[3] == 0x13) { // 睡眠模式关闭 (灯灭)
      voiceReddata = 255;
      voiceGreendata = 255;
      voiceBluedata = 255;
      voiceCommand = "Sleep model-c";
      Serial.println("语音命令：关闭睡眠模式");
      commandProcessed = true;
    }
    // 添加切换到手动模式的命令
    else if (receivedData[0] == 0xD2 && receivedData[1] == 0xD2 &&
             receivedData[2] == 0xD2 && receivedData[3] == 0xD2) { 
      Serial.println("语音命令：切换到手动模式");
      switchFromVoiceMode("manualMode");
      return; // 直接返回，因为已经不在语音模式了
    }
    // 添加切换到自动模式的命令
    else if (receivedData[0] == 0xD1 && receivedData[1] == 0xD1 &&
             receivedData[2] == 0xD1 && receivedData[3] == 0xD1) {
      Serial.println("语音命令：切换到自动模式");
      switchFromVoiceMode("autoMode");
      return; // 直接返回，因为已经不在语音模式了
    }
    else {
      Serial.println("无效的语音命令");
    }
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
      part = 0;
      sceneMode = "Reading";
      setManualModeState(part, sceneMode, READING_SCENE_R, READING_SCENE_G, READING_SCENE_B);
      manualModeEverEntered = true;
    } else {
      if (part == 0) {
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
void setManualModeState(int targetPart, String targetSceneMode, int r, int g, int b) {
    part = targetPart;
    sceneMode = targetSceneMode;
    manualRedValue = r;
    manualGreenValue = g;
    manualBlueValue = b;
    if (part == 0) {
        setSceneMode(sceneMode);
    } else {

        setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
    }
}
