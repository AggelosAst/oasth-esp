//
// Created by AggelosD on 4/6/2024.
//

#include <Arduino.h>
#include <ArduinoJson.h>

#ifndef OASTH_JSONPARSER_H
#define OASTH_JSONPARSER_H


enum direction {
    Return,
    Arrival,
    Unknown
};


class JSONParser {
public:
    static JsonDocument deserializeData(const char *data);

    static String serializeBusLocRequest();

    static String serializeConfRequest();

    static String serializePingRequest();
};


String JSONParser::serializePingRequest() {
    JsonDocument doc;
    String Serialized;

    doc["type"] = "ping";

    serializeJson(doc, Serialized);
    return Serialized;
}

String JSONParser::serializeConfRequest() {
    JsonDocument doc;
    String Serialized;

    doc["type"] = "get_conf";

    serializeJson(doc, Serialized);
    return Serialized;
}

String JSONParser::serializeBusLocRequest() {
    JsonDocument doc;
    String Serialized;

    doc["type"] = "get_bus_loc";

    serializeJson(doc, Serialized);
    return Serialized;
}


JsonDocument JSONParser::deserializeData(const char *data) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
    }
    return doc;
}


direction stringToDirection(const char *direction) {
    if (strcmp(direction, "Return") == 0) return Return;
    if (strcmp(direction, "Arrival") == 0) return Arrival;
    return Unknown;
}

#endif //OASTH_JSONPARSER_H
