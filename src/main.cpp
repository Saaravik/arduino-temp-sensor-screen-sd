#include <Arduino.h>

namespace {
constexpr unsigned long kBlinkIntervalMs = 1000;
unsigned long lastBlinkMs = 0;
bool ledOn = false;
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.begin(9600);
    Serial.println(F("Arduino Uno R3: proyecto iniciado"));
}

void loop() {
    const unsigned long now = millis();
    if (now - lastBlinkMs >= kBlinkIntervalMs) {
        lastBlinkMs = now;
        ledOn = !ledOn;
        digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);
    }
}
