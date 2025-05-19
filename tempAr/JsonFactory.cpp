#include "JsonFactory.h"

// AddToDoc重载实现
void AddToDoc(StaticJsonDocument<512>& doc, const String& key, int value) {
    doc[key] = value;
}

void AddToDoc(StaticJsonDocument<512>& doc, const String& key, bool value) {
    doc[key] = value;
}

void AddToDoc(StaticJsonDocument<512>& doc, const String& key, const char* value) {
    doc[key] = String(value);
}

void AddToDoc(StaticJsonDocument<512>& doc, const String& key, const String& value) {
    doc[key] = value;
}


