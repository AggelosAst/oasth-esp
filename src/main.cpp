#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "./libs/JSONParser.h"
#include "./libs/LedIndicator.h"

/* Initializers */

LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiMulti WiFiMulti;
HTTPClient http;
WebSocketsClient webSocket;


const constexpr char *ip = "domain"; /* Domain or IP */


const char *directionToString(direction Direction) {
    const char *Result;
    switch (Direction) {
        case Return: {
            Result = "Return";
            break;
        }
        case Arrival:
            Result = "Arrival";
            break;
        case Unknown:
            Result = "Unknown";
            break;
    }
    return Result;
}

void requestConf() {
    if (webSocket.isConnected()) {
        String SerializedPayload = JSONParser::serializeConfRequest();
        webSocket.sendTXT(SerializedPayload);
    }
}

void requestBusLoc() {
    if (webSocket.isConnected()) {
        String SerializedPayload = JSONParser::serializeBusLocRequest();
        webSocket.sendTXT(SerializedPayload);
    }
}

void ping() {
    if (webSocket.isConnected()) {
        String SerializedPayload = JSONParser::serializePingRequest();
        webSocket.sendTXT(SerializedPayload);
    }
}


char *lineId;
char *directionR;
bool stopped = false;

void Stop() {
    stopped = true;
    lcd.clear();
    lcd.setCursor(0, 1);

    lcd.print(" PAUSED");

    lcd.setCursor(0, 0);
    lcd.printf("L %s D %s", lineId, directionR);
}

void updateLineAndDirection(const char* LineId, direction Direction) {

    char* newLineId = static_cast<char*>(realloc(lineId, strlen(LineId) + 1));
    if (newLineId) {
        lineId = newLineId;
        strcpy(lineId, LineId);
    } else {
        Serial.println("[ERROR]: Memory reallocation for lineId failed");
    }

    const char* directionStr = directionToString(Direction);

    char* newDirectionR = static_cast<char*>(realloc(directionR, strlen(directionStr) + 1));
    if (newDirectionR) {
        directionR = newDirectionR;
        strcpy(directionR, directionStr);
    } else {
        Serial.println("[ERROR]: Memory reallocation for directionR failed");
    }
}

void webSocketEvent(WStype_t type, const uint8_t *payload, size_t length) {
    const char *Payload = (char *) payload;

    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("Disconnected");
            break;
        case WStype_CONNECTED:
            lcd.clear();
            lcd.print("Connected.");
            requestConf();
            break;
        case WStype_TEXT: {
            const JsonDocument jsonData = JSONParser::deserializeData(Payload);
            if (strcmp(jsonData["type"], "get_conf") == 0) {
                const char* LineId = jsonData["lineId"];
                direction Direction = stringToDirection(jsonData["direction"]);

                updateLineAndDirection(LineId, Direction);
                Serial.printf("[WS]: Line ID %s Direction %s \n", LineId, directionToString(Direction));
            } else if (strcmp(jsonData["type"], "get_bus_loc") == 0) {
                if (jsonData["error"] == true) {
                    const char *errormessage = jsonData["message"].as<const char *>();
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("ERROR!");
                    lcd.setCursor(0, 1);
                    lcd.print(String(errormessage).substring(0, 15));
                    LedIndicator::indicateBad();
                    delay(300);
                    LedIndicator::indicateNeutral();
                } else {
                    LedIndicator::indicateGood();
                    delay(100);
                    LedIndicator::indicateNeutral();
                    if (jsonData["no_bus"] == true) {
                        Serial.printf("[WS]: No bus\n");
                        lcd.clear();
                        lcd.setCursor(0, 0);
                        lcd.printf("L %s ", lineId);

                        lcd.setCursor(0, 1);
                        lcd.print("No Bus.");
                    } else if (jsonData["arrived"] == true) {
                        Serial.printf("[WS]: Bus arrived\n");
                        lcd.clear();
                        lcd.setCursor(0, 0);
                        lcd.printf("%s Route End", lineId);

                        lcd.setCursor(0, 1);
                        lcd.print(String(jsonData["destination"]["name"].as<const char *>()).substring(0, 14) +
                                  String(".."));
                    } else {
                        LedIndicator::indicateActiveRoute();
                        delay(100);
                        LedIndicator::indicateNeutral();
                        lcd.clear();
                        lcd.setCursor(0, 0);
                        lcd.printf("L %s N %s D %s", lineId, jsonData["vehicle"]["vehicle_number"].as<const char *>(),
                                   String(directionR).substring(0, 1).c_str());
                        lcd.setCursor(0, 1);
                        lcd.print(
                                String(jsonData["closest_bus_stop"]["StopDescrEng"].as<const char *>()).substring(
                                        0,
                                        14) +
                                String(".."));
                    }
                }
            } else if (strcmp(jsonData["type"], "ping") == 0) {
                Serial.println("[WS]: Received ping");
            }

            break;
        }
        case WStype_BIN:
            // ...
            break;
        case WStype_ERROR:
            break;
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
        case WStype_PING:
            break;
        case WStype_PONG:
            break;
    }

}

__attribute__((used)) void setup() {
    Serial.begin(9600);
    randomSeed(analogRead(0));
    pinMode(LedIndicator::redPin, OUTPUT);
    pinMode(LedIndicator::greenPin, OUTPUT);
    pinMode(LedIndicator::bluePin, OUTPUT);
    lcd.init();
    lcd.backlight();

    lcd.setCursor(0, 0);
    lcd.blink_on();
    lcd.print("Initialize...");

    WiFiMulti.addAP("SSID", "PASS");

    while (WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
        Serial.println("[WIFI]: Connecting to WiFi...");
    }

    Serial.println("[WIFI]: Connected to the network.");

    lcd.clear();
    lcd.print("Network av!");

    lcd.clear();
    lcd.print("Check serv...");

    http.begin(String("https://") + ip + String("/status"));
    http.setUserAgent("esp/1.0");
    http.setTimeout(1000);


    int httpCode = http.GET();

    if (httpCode != 200) {
        Serial.printf("[BACKEND]: Unusual status returned: %d\n", httpCode);
        lcd.clear();
        lcd.print("Serv down!");
        LedIndicator::indicateBadConstant();
    } else {
        Serial.printf("[BACKEND]: Server is Up! : %d\n", httpCode);
        lcd.clear();
        lcd.print("Serv up!");

        delay(100);

        lcd.clear();
        lcd.print("Socket conn...");

        webSocket.begin(ip, 8080, "/ws");
        webSocket.onEvent(webSocketEvent);
        webSocket.setReconnectInterval(5000);
    }
}

void loop() {
    webSocket.loop();
    static unsigned long lastSend = 0;
    static unsigned long lastPing = 0;

    int randomTime = random(10000, 15000);
//    Serial.printf("[REQ]: Random ms Time %d", randomTime);
    if (millis() - lastSend >= randomTime) {
        if (!stopped) {
            requestBusLoc();
            lastSend = millis();
        }
    }
    if (millis() - lastPing >= 5000) {
        if (!stopped) {
            ping();
            lastPing = millis();

        }
    }
}