#include "cross_room_control.h"
#include "state.h"
#include "light_control.h"

void handleCrossRoomCommand(const String& command) {
  // 只有在语音模式下才处理跨房间命令
  if (operationMode != "voiceMode") {
    Serial.println("非语音模式，忽略跨房间命令: " + command);
    return;
  }

  Serial.println("处理跨房间命令: " + command);

  // 处理各种命令
  if (command == "TURN_ON") {
    // 打开灯光（白光）
    voiceReddata = WHITE_MODE_R;
    voiceGreendata = WHITE_MODE_G;
    voiceBluedata = WHITE_MODE_B;
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  } 
  else if (command == "TURN_OFF") {
    // 关闭灯光
    voiceReddata = 255;
    voiceGreendata = 255;
    voiceBluedata = 255;
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  }
  else if (command == "BRIGHTNESS_UP") {
    // 调亮
    if(voiceReddata == voiceGreendata && voiceBluedata == voiceGreendata) { // 灰度/白光
      int currentBrightness = voiceReddata;
      currentBrightness = max(0, currentBrightness - 40);
      voiceReddata = voiceGreendata = voiceBluedata = currentBrightness;
    } else { // 彩色光
      voiceReddata = max(0, (int)(voiceReddata * 0.8));
      voiceGreendata = max(0, (int)(voiceGreendata * 0.8));
      voiceBluedata = max(0, (int)(voiceBluedata * 0.8));
    }
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  }
  else if (command == "BRIGHTNESS_DOWN") {
    // 调暗
    if(voiceReddata == voiceGreendata && voiceBluedata == voiceGreendata) { // 灰度/白光
      int currentBrightness = voiceReddata;
      currentBrightness = min(255, currentBrightness + 40);
      voiceReddata = voiceGreendata = voiceBluedata = currentBrightness;
    } else { // 彩色光
      voiceReddata = min(255, (int)(voiceReddata / 0.8));
      voiceGreendata = min(255, (int)(voiceGreendata / 0.8));
      voiceBluedata = min(255, (int)(voiceBluedata / 0.8));
    }
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  }
  else if (command == "MODE_FILM") {
    // 影视模式
    voiceReddata = FILM_SCENE_R;
    voiceGreendata = FILM_SCENE_G;
    voiceBluedata = FILM_SCENE_B;
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  }
  else if (command == "MODE_PARTY") {
    // 派对模式
    voiceReddata = PARTY_SCENE_R;
    voiceGreendata = PARTY_SCENE_G;
    voiceBluedata = PARTY_SCENE_B;
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  }
  else if (command == "MODE_READING") {
    // 阅读模式
    voiceReddata = READING_SCENE_R;
    voiceGreendata = READING_SCENE_G;
    voiceBluedata = READING_SCENE_B;
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  }
  else if (command == "MODE_SLEEPING") {
    // 睡眠模式
    voiceReddata = SLEEPING_SCENE_R;
    voiceGreendata = SLEEPING_SCENE_G;
    voiceBluedata = SLEEPING_SCENE_B;
    setLEDColor(voiceReddata, voiceGreendata, voiceBluedata);
  }
}

void sendCrossRoomCommand(const String& targetRoom, const String& command) {
  // 创建JSON消息
  StaticJsonDocument<256> doc;
  doc["type"] = "voice_command";
  doc["command"] = command;
  doc["room"] = DEVICE_ID; // 发送者房间ID
  
  String jsonMsg;
  serializeJson(doc, jsonMsg);
  
  // 根据目标房间找到对应的ESP ID
  String targetESP = "";
  if (targetRoom == "bedroom") {
    targetESP = "ESP_BEDROOM";
  } else if (targetRoom == "living_room") {
    targetESP = "ESP_LIVING_ROOM";
  }
  
  if (targetESP == "") {
    Serial.println("未知目标房间: " + targetRoom);
    return;
  }
  
  // 发送消息
  if (sendToPeer(targetESP, jsonMsg)) {
    Serial.println("已发送跨房间命令到" + targetRoom + ": " + command);
  } else {
    Serial.println("发送跨房间命令失败: " + command);
  }
}

bool parseCrossRoomMessage(const String& message, String& command) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print(F("解析JSON失败: "));
    Serial.println(error.f_str());
    return false;
  }
  
  // 检查消息类型
  if (!doc.containsKey("type") || doc["type"] != "voice_command") {
    return false;
  }
  
  // 提取命令
  if (doc.containsKey("command")) {
    command = doc["command"].as<String>();
    return true;
  }
  
  return false;
}
