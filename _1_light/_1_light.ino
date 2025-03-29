#include <ArduinoJson.h>  // 需要安装ArduinoJson库

const int photoSensorPin = A0;  // 光敏模块的模拟输出A0接Arduino的A0
const int redPin   = 9;         // 红色通道，PWM引脚
const int greenPin = 10;        // 绿色通道，PWM引脚
const int bluePin  = 11;        // 蓝色通道，PWM引脚

// 操作模式和灯光模式
String operationMode = "Manual";  // 操作模式: "Auto" 或 "Manual"
String lightMode = "Custom";      // 灯光模式: "Bright", "Warm", "Night", "Custom"

// 注意：灯的逻辑是反的，0是最亮，255是全暗
int manualRedValue = 0;       // 手动模式默认灯光值（全亮）
int manualGreenValue = 0;
int manualBlueValue = 0;

// 预设灯光模式的RGB值（反转逻辑：0是最亮，255是全暗）
const int BRIGHT_MODE_R = 0, BRIGHT_MODE_G = 0, BRIGHT_MODE_B = 0;        // 明亮模式 - 纯白光
const int WARM_MODE_R = 0, WARM_MODE_G = 75, WARM_MODE_B = 155;           // 温暖模式 - 暖黄色光
const int NIGHT_MODE_R = 205, NIGHT_MODE_G = 205, NIGHT_MODE_B = 105;     // 夜灯模式 - 柔和蓝光

void setup() {
  Serial.begin(9600);           // 初始化串口，用于与ESP8266通信
  
  // 设置RGB LED各通道为输出
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  
  // 初始状态：灯光开启（灯光逻辑反转，0是最亮）
  setLEDColor(0, 0, 0);
  
  Serial.println("Arduino准备就绪，等待ESP8266的命令...");
}

void loop() {
  // 检查是否有来自ESP8266的消息
  checkSerialCommand();
  
  // 如果是自动调节模式，则按照光敏传感器值调整灯光
  if (operationMode == "Auto") {
    adjustLightingBasedOnSensor();
  }
  
  delay(100);  // 延时100ms，便于观察效果
}

// 设置预设灯光模式
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
  
  Serial.print("已设置灯光模式: ");
  Serial.println(lightMode);
  
  // 如果模式发生变化，立即发送状态更新
  if (oldMode != lightMode) {
    sendResponseToESP(); // 直接发送状态更新
  }
}


// 设置系统操作模式
void setOperationMode(const String& mode) {
  String oldMode = operationMode;
  
  if (mode == "Auto") {
    operationMode = "Auto";
    lightMode = "Auto";  // 自动模式下灯光模式标记为Auto
    Serial.println("已切换到自动调节模式");
  } 
  else if (mode == "Manual") {
    operationMode = "Manual";
    // 进入手动模式时保持当前RGB设置，但如果之前是Auto则设为Custom
    if (oldMode == "Auto") {
      lightMode = "Custom";
    }
    Serial.println("已切换到手动控制模式");
  }
  // 其他可能的操作模式可以在这里添加...
  else {
    Serial.print("未知的操作模式: ");
    Serial.println(mode);
    return;  // 不更改模式
  }
  
  // 如果切换到手动模式，应用当前的手动RGB设置
  if (operationMode == "Manual") {
    setLEDColor(manualRedValue, manualGreenValue, manualBlueValue);
  }
}

// 检查并处理来自ESP8266的命令
void checkSerialCommand() {
  if (Serial.available()) {
    // 读取整行命令
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    Serial.print("收到命令: ");
    Serial.println(command);
    
    // 创建JSON文档对象
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, command);
    
    // 检查是否成功解析JSON
    if (error) {
      Serial.print("JSON解析失败: ");
      Serial.println(error.c_str());
      return;
    }
    
    bool stateChanged = false;
    
    // 检查是否是状态查询命令
    if (doc.containsKey("Query") && doc["Query"].as<String>() == "Status") {
      Serial.println("收到状态查询请求");
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
      Serial.print("设置手动灯光: R=");
      Serial.print(manualRedValue);
      Serial.print(", G=");
      Serial.print(manualGreenValue);
      Serial.print(", B=");
      Serial.println(manualBlueValue);
    }
    
    // 仅当状态发生变化时才发送响应
    if (stateChanged) {
      sendResponseToESP();
    }
  }
}

// 基于光敏传感器调整灯光
// 基于光敏传感器调整灯光 - 移除所有调试输出
void adjustLightingBasedOnSensor() {
  // 读取光敏模块数值（0~1023）
  int sensorValue = analogRead(photoSensorPin);
  
  // 将传感器值转换为百分比（0~100%）
  int percentage = map(sensorValue, 0, 1023, 100, 0);

  // 移除所有光照强度输出
  // Serial.print("光照强度: "); - 已删除
  // Serial.print(percentage);   - 已删除
  // Serial.println("%");        - 已删除

  // 计算PWM值（反转逻辑：0是最亮，255是全暗）
  int pwmValue = map(sensorValue, 0, 1023, 255,0) *1.1;
  if (percentage >90) pwmValue = 255;
  if (pwmValue < 0) pwmValue = 0;
  
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
// 向ESP8266发送当前状态
void sendResponseToESP() {
  StaticJsonDocument<256> doc;
  
  doc["OperationMode"] = operationMode;
  doc["LightMode"] = lightMode;
  
  // 删除自动模式下的光照级别信息
  if (operationMode == "Manual") {
    doc["Red"] = manualRedValue;
    doc["Green"] = manualGreenValue;
    doc["Blue"] = manualBlueValue;
  }
  // 移除了Auto模式下发送LightLevel的代码
  
  String response;
  serializeJson(doc, response);
  
  // 确保消息完整地发送
  Serial.println(response);
  delay(10); // 给串口缓冲区一点时间
}
