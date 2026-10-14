# Arduino Uno R3: sensor de temperatura, pantalla y SD

Proyecto base para Arduino Uno R3 (ATmega328P), con PlatformIO y el framework Arduino.

Repositorio: https://github.com/Saaravik/arduino-temp-sensor-screen-sd

## Estructura

```text
.
├── platformio.ini         # Configuración de Arduino Uno y puerto serie
├── src/main.cpp           # Programa principal: setup() y loop()
├── include/               # Cabeceras compartidas
├── lib/                   # Librerías propias
├── test/                  # Pruebas
└── .vscode/extensions.json
```

## Uso

Instala PlatformIO IDE en Visual Studio Code y abre esta carpeta. También puedes usar PlatformIO Core desde la terminal:

```sh
pio run                         # Compilar
pio run --target upload         # Cargar el programa por USB
pio device monitor              # Monitor serie a 9600 baudios
```

PlatformIO detecta el puerto USB automáticamente. Si hay varios dispositivos, especifica `upload_port` y `monitor_port` en `platformio.ini`.

El programa inicial cambia el estado del LED integrado cada segundo y envía un mensaje de inicio por el puerto serie. No requiere componentes externos.

## Sensor, pantalla y tarjeta SD

El código de los periféricos se añadirá cuando se definan sus modelos y conexiones. Declara sus librerías en `lib_deps` dentro del entorno `[env:uno]` de `platformio.ini`.

## Git

El remoto `origin` apunta al repositorio indicado. Para publicar el primer commit desde la carpeta del proyecto:

```sh
git add .
git commit -m "Configura proyecto PlatformIO para Arduino Uno R3"
git push -u origin HEAD
```
