// light_control.h
#ifndef LIGHT_CONTROL_H
#define LIGHT_CONTROL_H

#include <Arduino.h> // for analogWrite, Serial, constrain, map, max, min
#include "config.h" // 包含配置信息 (如引脚定义, RGB常量)
#include "state.h"  // 包含状态变量
#include "mqtt_handler.h"
#include "ConnectionManager.h"
void setLEDColor(int red, int green, int blue);
void setSceneMode(const String& mode);
void setBaseColorForManualMode(const String& colorName);
void adjustBrightnessForManualMode(float level);
void adjustLightingBasedOnSensor();
void checkVoiceCommands();
void setOperationMode(const String& newMode);
void setManualModeState(bool targetPart, String targetSceneMode, int r, int g, int b);
void handlePartChange(bool newPart);
void updatePartyLights();
void updateStrobeEffect();
void hsvToRgb(int h, int s, int v, int &r, int &g, int &b);
void handleRemoteCommand(const String& message);
#endif
