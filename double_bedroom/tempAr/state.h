// state.h
#ifndef STATE_H
#define STATE_H

#include <Arduino.h> // 需要String类型

// 灯光控制变量
extern String operationMode;  // 操作模式: "autoMode", "manualMode", "voiceMode"
extern String voiceCommand;     // 当前语音命令

// part 表征手动模式下的子模式: 0 (预设情景模式) 或 1 (颜色+亮度调节)
extern bool part; // 默认设置为预设情景模式 (0: sceneMode)

//手动模式下part == 0 时，预设情景模式sceneMode
extern String sceneMode; //sceneMode用于表示手动模式下的预设模式

// 语音模式RGB值
extern int voiceReddata;
extern int voiceGreendata;
extern int voiceBluedata;

// 手动模式RGB值
extern int manualRedValue;
extern int manualGreenValue;
extern int manualBlueValue;

// 状态变量
extern unsigned long lastStatusUpdate;
extern unsigned long lastReconnectAttempt;


// part为1下的颜色基准，用于亮度调节
extern int currentColorBaseR;
extern int currentColorBaseG;
extern int currentColorBaseB;
extern String currentBaseColorName; // 默认值
extern float currentBrightnessLevel; // 1-5 级，初始为 1

// 手动模式自启动以来是否被进入过
extern bool manualModeEverEntered;

//party模式渐变
extern unsigned long lastPartyUpdate;
extern int partyHue; // 0-360度的HSV色相值
extern bool partyModeActive;
//party模式频闪
extern unsigned long lastStrobeUpdate;
extern bool strobeState;
extern int strobeSpeed;
extern int strobeColor;
extern int strobeColorHue;

extern bool partyModeActive;
#endif
