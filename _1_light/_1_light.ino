#include <ArduinoJson.h>
#include <SoftwareSerial.h>

// 引脚定义
const int photoSensorPin = A0;  // 光敏模块的模拟输出A0接Arduino的A0
const int redPin = 9;           // 红色通道，PWM引脚
const int greenPin = 10;        // 绿色通道，PWM引脚
const int bluePin = 11;         // 蓝色通道，PWM引脚

// 创建软件串口用于ESP8266通信
SoftwareSerial espSerial(5, 6);  // RX, TX (连接ESP8266)

// 操作模式和灯光模式
String operationMode = "Manual";  // 操作模式: "Auto", "Manual", "Voice"
String lightMode = "Custom";      // 灯光模式: "Bright", "Warm", "Night", "Custom", "Auto", "Voice"
String voiceCommand = "None";     // 当前语音命令

// 注意：灯的逻辑是反的，0是最亮，255是全暗
int manualRedValue = 0;       // 手动模式默认灯光值（全亮）
int manualGreenValue = 0;
int manualBlueValue = 0;

// 预设灯光模式的RGB值（反转逻辑：0是最亮，255是全暗）
const int BRIGHT_MODE_R = 0, BRIGHT_MODE_G = 0, BRIGHT_MODE_B = 0;        // 明亮模式 - 纯白光
const int WARM_MODE_R = 0, WARM_MODE_G = 75, WARM_MODE_B = 155;           // 温暖模式 - 暖黄色光
const int NIGHT_MODE_R = 205, NIGHT_MODE_G = 205, NIGHT_MODE_B = 105;     // 夜灯模式 - 柔和蓝光

// 上次状态更新时间
unsigned long lastStatusUpdate = 0;
// 是否有状态变化需要上报
bool statusChanged = false;

void setup() {
  Serial.begin(9600);           // 硬件串口用于语音模块
  espSerial.begin(9600);        // 软件串口用于ESP8266通信
  
  // 设置RGB LED各通道为输出
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  
  // 初始状态：灯光开启（灯光逻辑反转，0是最亮）
  setLEDColor(0, 0, 0);
  
  espSerial.println("Arduino准备就绪，等待命令...");
  
  // 调试信息，使用硬件串口输出
  Serial.println("Arduino启动完成");
}

void loop() {
  // 检查是否有来自ESP8266的消息
  checkESPCommand();
  
  // 根据操作模式执行不同的逻辑
  if (operationMode == "Auto") {
    adjustLightingBasedOnSensor();
  } 
  else if (operationMode == "Voice") {
    checkVoiceCommands();
  }
  
  // 定期发送状态更新（如果有变化）
  if (statusChanged && (millis() - lastStatusUpdate > 1000)) {
    sendResponseToESP();
    statusChanged = false;
    lastStatusUpdate = millis();
  }
  
  delay(50);  // 短暂延时，减轻处理负担
}

// 检查ESP8266命令
void checkESPCommand() {
  if (espSerial.available()) {
    // 读取整行命令
    String command = espSerial.readStringUntil('\n');
    command.trim();
    
    // 调试输出
    Serial.print("收到ESP命令: ");  // 调试到硬件串口
    Serial.println(command);
    
    // 创建JSON文档对象
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, command);
    
    // 检查是否成功解析JSON
    if (error) {
      espSerial.print("JSON解析失败: ");
      espSerial.println(error.c_str());
      Serial.print("JSON解析失败: ");  // 调试到硬件串口
      Serial.println(error.c_str());
      return;
    }
    
    bool stateChanged = false;
    
    // 检查是否是状态查询命令
    if (doc.containsKey("Query") && doc["Query"].as<String>() == "Status") {
      espSerial.println("收到状态查询请求");
      sendResponseToESP();
      return;
    }
    
    // 检查是否包含操作模式命令
    if (doc.containsKey("OperationMode")) {
      String modeValue = doc["OperationMode"].as<String>();
      String oldMode = operationMode;
      
      setOperationMode(modeValue);
      
      if (oldMode != operationMode) {
        stateChanged = true;
      }
    }
    
    // 向后兼容：检查是否包含AutoAdjust指令
    if (doc.containsKey("AutoAdjust")) {
      String autoAdjustValue = doc["AutoAdjust"].as<String>();
      String oldMode = operationMode;
      
      if (autoAdjustValue == "True") {
        setOperationMode("Auto");
      } else if (autoAdjustValue == "False") {
        setOperationMode("Manual");
      }
      
      if (oldMode != operationMode) {
        stateChanged = true;
      }
    }
    
    // 检查是否包含灯光模式命令 (仅在手动模式下有效)
    if (doc.containsKey("LightMode") && operationMode == "Manual") {
      String modeValue = doc["LightMode"].as<String>();
      
      String oldMode = lightMode;
      setLightMode(modeValue);
      
      // 检测模式是否改变
      if (oldMode != lightMode) {
        stateChanged = true;
      }
    }
    
    // 检查是否包含手动RGB设置 (仅在手动模式下有效)
    if (operationMode == "Manual" && 
       (doc.containsKey("Red") || doc.containsKey("Green") || doc.containsKey("Blue"))) {
      // 如果有任何RGB值被单独设置，模式变为Custom
      if (lightMode != "Custom") {
        lightMode = "Custom";
        stateChanged = true;
      }
      
      if (doc.containsKey("Red")) {
        int oldRed = manualRedValue;
        manualRedValue = doc["Red"].as<int>();
        if (oldRed != manualRedValue) stateChanged = true;
      }
      if (doc.containsKey("Green")) {
        int oldGreen = manualGreenValue;
        manualGreenValue = doc["Green"].as<int>();
        if (oldGreen != manualGreenValue) stateChanged = true;
      }
      if (doc.containsKey("Blue")) {
        int oldBlue = manualBlueValue;
        manualBlueValue = doc["Blue"].as<int>();
        if (oldBlue != manualBlueValue) stateChanged = true;
      }
      
      // 应用手动设置的RGB值
      setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
      espSerial.print("设置手动灯光: R=");
      espSerial.print(manualRedValue);
      espSerial.print(", G=");
      espSerial.print(manualGreenValue);
      espSerial.print(", B=");
      espSerial.println(manualBlueValue);
    }
    
    // 仅当状态发生变化时才标记为需要更新
    if (stateChanged) {
      statusChanged = true;
    }
  }
}

// 检查语音命令 - 使用硬件串口
void checkVoiceCommands() {
  if (Serial.available() >= 4) {  // 确保至少收到4个字节
    byte receivedData[4];
    Serial.readBytes(receivedData, 4);  // 读取4个字节
    
    String oldCommand = voiceCommand;
    String commandHex = "";
    
    for (int i = 0; i < 4; i++) {
      char hexStr[3];
      sprintf(hexStr, "%02X", receivedData[i]);
      commandHex += hexStr;
    }
    
    // 输出到ESP串口以便于MQTT上报
    espSerial.print("收到语音指令(HEX): ");
    espSerial.println(commandHex);
    
    // 判断接收到的命令
    if (receivedData[0] == 0xA1 && receivedData[1] == 0xA1 &&
        receivedData[2] == 0xA1 && receivedData[3] == 0xA1) {
      // 蓝灯 - 可以映射为Night模式
      setLEDColor(255, 255, 0);  // 蓝灯亮(0是最亮)
      voiceCommand = "Blue";
      lightMode = "Voice-Blue";
      espSerial.println("语音命令：蓝灯开启");
    }
    else if (receivedData[0] == 0xA2 && receivedData[1] == 0xA2 &&
             receivedData[2] == 0xA2 && receivedData[3] == 0xA2) {
      // 红灯 - 可以映射为Warm模式
      setLEDColor(0, 255, 255);  // 红灯亮(0是最亮)
      voiceCommand = "Red";
      lightMode = "Voice-Red";
      espSerial.println("语音命令：红灯开启");
    }
    else if (receivedData[0] == 0xA3 && receivedData[1] == 0xA3 &&
             receivedData[2] == 0xA3 && receivedData[3] == 0xA3) {
      // 绿灯 - 自定义模式
      setLEDColor(255, 0, 255);  // 绿灯亮(0是最亮)
      voiceCommand = "Green";
      lightMode = "Voice-Green";
      espSerial.println("语音命令：绿灯开启");
    }
    else {
      // 无效命令 - 不改变当前状态
      espSerial.println("无效的语音命令");
    }
    
    // 如果命令有变化，标记需要上报状态
    if (oldCommand != voiceCommand) {
      statusChanged = true;
    }
  }
}

// 设置预设灯光模式
void setLightMode(const String& mode) {
  String oldMode = lightMode; // 保存旧模式用于对比
  lightMode = mode;
  
  if (mode == "Bright") {
    // 明亮模式 - 纯白光
    manualRedValue = BRIGHT_MODE_R;
    manualGreenValue = BRIGHT_MODE_G;
    manualBlueValue = BRIGHT_MODE_B;
    
  } else if (mode == "Warm") {
    // 温暖模式 - 暖黄色光
    manualRedValue = WARM_MODE_R;
    manualGreenValue = WARM_MODE_G;
    manualBlueValue = WARM_MODE_B;
    
  } else if (mode == "Night") {
    // 夜灯模式 - 柔和蓝光
    manualRedValue = NIGHT_MODE_R;
    manualGreenValue = NIGHT_MODE_G;
    manualBlueValue = NIGHT_MODE_B;
    
  } else {
    // 未知模式，保持当前值不变
    lightMode = "Custom";
  }
  
  // 应用新的灯光设置
  setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
  
  espSerial.print("已设置灯光模式: ");
  espSerial.println(lightMode);
  
  // 如果模式发生变化，立即发送状态更新
  if (oldMode != lightMode) {
    statusChanged = true;
  }
}

// 设置系统操作模式
void setOperationMode(const String& mode) {
  String oldMode = operationMode;
  
  if (mode == "Auto") {
    operationMode = "Auto";
    lightMode = "Auto";  // 自动模式下灯光模式标记为Auto
    espSerial.println("已切换到自动调节模式");
  } 
  else if (mode == "Manual") {
    operationMode = "Manual";
    // 进入手动模式时保持当前RGB设置，但如果之前是Auto或Voice则设为Custom
    if (oldMode == "Auto" || oldMode == "Voice") {
      lightMode = "Custom";
    }
    espSerial.println("已切换到手动控制模式");
  }
  else if (mode == "Voice") {
    operationMode = "Voice";
    lightMode = "Voice";  // 语音模式下灯光模式标记为Voice
    voiceCommand = "None"; // 重置语音命令状态
    espSerial.println("已切换到语音控制模式");
  }
  else {
    espSerial.print("未知的操作模式: ");
    espSerial.println(mode);
    return;  // 不更改模式
  }
  
  // 如果切换到手动模式，应用当前的手动RGB设置
  if (operationMode == "Manual") {
    setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
  }
  
  // 如果模式发生变化，标记需要更新状态
  if (oldMode != operationMode) {
    statusChanged = true;
  }
}

// 基于光敏传感器调整灯光
void adjustLightingBasedOnSensor() {
  // 读取光敏模块数值（0~1023）
  int sensorValue = analogRead(photoSensorPin);
  
  // 将传感器值转换为百分比（0~100%）
  int percentage = map(sensorValue, 0, 1023, 100, 0);
  
  // 计算PWM值 - 灯光逻辑是反的（0是最亮，255是最暗）
  int pwmValue = map(sensorValue, 0, 1023, 255, 0);
  
  // 微调，确保在完全黑暗时灯光最亮
  if (percentage > 90) {
    pwmValue = 255; // 最亮
  }
  
  // 确保PWM值在有效范围内
  if (pwmValue < 0) pwmValue = 0;
  if (pwmValue > 255) pwmValue = 255;
  
  // 设置RGB值
  int redValue = pwmValue;
  int greenValue = pwmValue;
  int blueValue = pwmValue;
  
  // 通过PWM调节RGB通道
  setLEDColor(redValue, greenValue, blueValue);
}

// 设置LED颜色的辅助函数
void setLEDColor(int red, int green, int blue) {
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}

// 向ESP8266发送当前状态
void sendResponseToESP() {
  StaticJsonDocument<256> doc;
  
  doc["OperationMode"] = operationMode;
  doc["LightMode"] = lightMode;
  
  // 根据不同操作模式添加特定信息
  if (operationMode == "Manual") {
    doc["Red"] = manualRedValue;
    doc["Green"] = manualGreenValue;
    doc["Blue"] = manualBlueValue;
  } 
  else if (operationMode == "Voice") {
    doc["VoiceCommand"] = voiceCommand;
    // 同时也发送当前的RGB值，方便UI显示
    if (voiceCommand == "Blue") {
      doc["Red"] = 255;
      doc["Green"] = 255;
      doc["Blue"] = 0;
    } 
    else if (voiceCommand == "Red") {
      doc["Red"] = 0;
      doc["Green"] = 255;
      doc["Blue"] = 255;
    }
    else if (voiceCommand == "Green") {
      doc["Red"] = 255;
      doc["Green"] = 0;
      doc["Blue"] = 255;
    }
  }
  
  String response;
  serializeJson(doc, response);
  
  // 通过软件串口发送给ESP8266
  espSerial.println(response);
  delay(10); // 给串口缓冲区一点时间
  
  // 调试输出
  Serial.print("发送状态到ESP: ");
  Serial.println(response);
  
  // 重置状态变化标志
  lastStatusUpdate = millis();
  statusChanged = false;
}
