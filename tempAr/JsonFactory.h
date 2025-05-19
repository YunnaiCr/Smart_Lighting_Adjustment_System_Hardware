#ifndef JSONFACTORY_H
#define JSONFACTORY_H

#include <ArduinoJson.h>

// 函数声明
void AddToDoc(StaticJsonDocument<512>& doc, const String& key, int value);
void AddToDoc(StaticJsonDocument<512>& doc, const String& key, bool value);
void AddToDoc(StaticJsonDocument<512>& doc, const String& key, const char* value);
void AddToDoc(StaticJsonDocument<512>& doc, const String& key, const String& value);

// 递归终止条件 - 直接在头文件中实现模板函数
inline void AddToDocRecursive(StaticJsonDocument<512>& doc) {
    // 递归终止，不执行任何操作
}

// 处理单个键值对
template <typename T>
void AddToDocRecursive(StaticJsonDocument<512>& doc, const char* key, T value) {
    AddToDoc(doc, key, value);
}

// 处理多个键值对的递归模板
template <typename T, typename... Args>
void AddToDocRecursive(StaticJsonDocument<512>& doc, const char* key, T value, Args... args) {
    AddToDoc(doc, key, value);
    AddToDocRecursive(doc, args...);
}

/*
  If you would like to converse data, please input data in pairs.
    String result = ConverseDataToJson("FirstKey", 12, "SecondKey", true);
*/
template<typename... Args>
String ConverseDataToJson(Args... args) {
    StaticJsonDocument<512> doc;
    static_assert(sizeof...(args) % 2 == 0, "Parameters must be passed in pairs.");
    AddToDocRecursive(doc, args...);

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

#endif
