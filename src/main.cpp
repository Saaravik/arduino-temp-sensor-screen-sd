#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <RTClib.h>
#include <SdFat.h>
#include <U8x8lib.h>
#include <math.h>

namespace {
constexpr uint8_t kSdCsPin = 10;
constexpr unsigned long kSampleIntervalMs = 1000;
constexpr unsigned long kLogIntervalMs = 60000;
constexpr char kLogFile[] = "DATOS.CSV";

// Modo texto: evita reservar un framebuffer en los 2 KB de RAM del Uno.
U8X8_SSD1306_128X32_UNIVISION_HW_I2C display(U8X8_PIN_NONE);
Adafruit_BME280 bme;
SdExFat sd;
RTC_Millis clock;
bool displayReady = false;
bool sensorReady = false;
bool sampleValid = false;
bool sdReady = false;
const __FlashStringHelper *sdStatus = nullptr; // Se establece al inicializar la SD.
float temperature = 0;
float humidity = 0;
float pressure = 0;
unsigned long lastSampleMs = 0;
unsigned long lastLogMs = 0;

bool respondsAt(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

bool startSensor() {
    if (!bme.begin(0x76) && !bme.begin(0x77)) {
        return false;
    }
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1,
                    Adafruit_BME280::SAMPLING_X1,
                    Adafruit_BME280::SAMPLING_X1,
                    Adafruit_BME280::FILTER_OFF);
    return true;
}

void readSample() {
    if (!sensorReady) {
        sensorReady = startSensor();
    }
    sampleValid = false;
    if (sensorReady && bme.takeForcedMeasurement()) {
        temperature = bme.readTemperature();
        humidity = bme.readHumidity();
        pressure = bme.readPressure() / 100.0F; // Pa -> hPa, presion local.
        sampleValid = isfinite(temperature) && isfinite(humidity) &&
                      isfinite(pressure) && temperature >= -40 &&
                      temperature <= 85 && humidity >= 0 && humidity <= 100 &&
                      pressure >= 300 && pressure <= 1100;
    }
    if (!sampleValid) {
        sensorReady = false;
        Serial.println(F("Error BME280: muestra omitida"));
    }
}

void updateDisplay() {
    if (!displayReady) {
        return;
    }
    // Cada fila dispone de 16 caracteres; limpiar evita restos de cifras.
    display.clearLine(0);
    display.setCursor(0, 0);
    if (sampleValid) {
        display.print(F("T: "));
        display.print(temperature, 1);
        display.print(F(" C"));
        display.clearLine(1);
        display.setCursor(0, 1);
        display.print(F("H: "));
        display.print(humidity, 1);
        display.print(F(" %"));
        display.clearLine(2);
        display.setCursor(0, 2);
        display.print(F("P: "));
        display.print(pressure, 1);
        display.print(F(" hPa"));
    } else {
        display.print(F("Error BME280"));
        display.clearLine(1);
        display.clearLine(2);
    }
    display.clearLine(3);
    display.setCursor(0, 3);
    display.print(sdReady ? F("SD OK Hora aprox") : sdStatus);
}

bool sdFailure(const __FlashStringHelper *status) {
    sdStatus = status;
    Serial.println(status);
    sd.errorPrint(&Serial);
    return false;
}

bool startSd() {
    pinMode(kSdCsPin, OUTPUT);
    digitalWrite(kSdCsPin, HIGH); // CS activo en LOW: comenzar deseleccionado.
    // Velocidad conservadora para diagnosticar cableado y modulos con adaptacion.
    if (!sd.cardBegin(SdSpiConfig(kSdCsPin, SHARED_SPI, SD_SCK_MHZ(1)))) {
        return sdFailure(F("SD SPI ERROR"));
    }
    if (!sd.volumeBegin()) {
        return sdFailure(F("SD exFAT ERROR"));
    }
    Serial.println(F("SD: SPI y volumen exFAT OK"));
    return true;
}

void printTwoDigits(Print &output, uint8_t value) {
    if (value < 10) {
        output.write('0');
    }
    output.print(value);
}

bool appendLog() {
    // Reintenta montar la tarjeta en el siguiente registro tras un fallo.
    if (!sdReady && !startSd()) {
        return false;
    }
    ExFile file;
    if (!file.open(kLogFile, O_WRONLY | O_CREAT | O_AT_END)) {
        return sdFailure(F("SD FILE ERROR"));
    }
    if (file.fileSize() == 0) {
        file.println(F("fecha,temperatura_C,humedad_pct,presion_hPa"));
    }
    const DateTime now = clock.now();
    file.print(now.year());
    file.write('-');
    printTwoDigits(file, now.month());
    file.write('-');
    printTwoDigits(file, now.day());
    file.write('T');
    printTwoDigits(file, now.hour());
    file.write(':');
    printTwoDigits(file, now.minute());
    file.write(':');
    printTwoDigits(file, now.second());
    file.write(',');
    file.print(temperature, 2);
    file.write(',');
    file.print(humidity, 2);
    file.write(',');
    file.println(pressure, 2);
    const bool written = !file.getWriteError() && file.sync();
    const bool closed = file.close();
    if (!written || !closed) {
        return sdFailure(F("SD WRITE ERROR"));
    }
    return true;
}
} // namespace

void setup() {
    Serial.begin(9600);
    Wire.begin();
    Wire.setWireTimeout(25000, true);
    clock.begin(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println(F("Reloj provisional: fecha/hora de compilacion; se reinicia al arrancar"));

    const uint8_t oledAddress = respondsAt(0x3C) ? 0x3C :
                                (respondsAt(0x3D) ? 0x3D : 0);
    if (oledAddress != 0) {
        display.setI2CAddress(oledAddress << 1);
        display.begin();
        display.setFont(u8x8_font_chroma48medium8_r);
        displayReady = true;
    } else {
        Serial.println(F("OLED no detectada en 0x3C/0x3D"));
    }
    sensorReady = startSensor();
    sdReady = startSd();
    readSample();
    updateDisplay();
    lastSampleMs = lastLogMs = millis();
}

void loop() {
    // Llamar a menudo permite al reloj software gestionar el rollover de millis().
    clock.now();
    const unsigned long now = millis();
    const bool logDue = now - lastLogMs >= kLogIntervalMs;
    if (now - lastSampleMs >= kSampleIntervalMs || logDue) {
        lastSampleMs = now;
        readSample();
        if (logDue) {
            lastLogMs = now;
            if (sampleValid) {
                sdReady = appendLog();
                Serial.println(sdReady ? F("CSV: muestra guardada") :
                                         F("Error SD: muestra no guardada"));
            }
        }
        updateDisplay();
    }
}
