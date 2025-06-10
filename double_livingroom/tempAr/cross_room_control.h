#ifndef CROSS_ROOM_CONTROL_H
#define CROSS_ROOM_CONTROL_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"
#include "ConnectionManager.h"

// 处理来自其他房间的命令
void handleCrossRoomCommand(const String& command);

// 发送命令到其他房间
void sendCrossRoomCommand(const String& targetRoom, const String& command);

// 解析JSON格式的跨房间命令
bool parseCrossRoomMessage(const String& message, String& command);

#endif