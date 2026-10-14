# Arduino Uno R3: BME280, OLED y registro SD

Lee temperatura, humedad y presión del BME280 cada segundo y muestra los valores en una OLED SSD1306 I2C de 128×32. Cada minuto añade la última lectura válida a `DATOS.CSV` en una tarjeta SD exFAT.

Repositorio: https://github.com/Saaravik/arduino-temp-sensor-screen-sd

## Conexiones

Conecta los módulos con la alimentación desconectada y comparte GND entre todos ellos.

| Módulo | Pin del módulo | Arduino Uno R3 |
| --- | --- | --- |
| OLED SSD1306 | SDA | A4 / SDA |
| OLED SSD1306 | SCL | A5 / SCL |
| BME280 | SDA | A4 / SDA |
| BME280 | SCL | A5 / SCL |
| SD | CS | D10 |
| SD MikroE HW REV. 1.00 | SDO (salida del microcontrolador, MOSI) | D11 |
| SD MikroE HW REV. 1.00 | SDI (entrada del microcontrolador, MISO) | D12 |
| SD | SCK / CLK (marcado SDK según la descripción) | D13 |
| Todos | GND | GND |

OLED y BME280 comparten el bus I2C. Se detectan las direcciones OLED `0x3C`/`0x3D` y BME280 `0x76`/`0x77` automáticamente. Si el BME280 tiene CS y SDO, configura CS a su alimentación lógica para seleccionar I2C y SDO a GND o a su alimentación lógica para elegir la dirección; no lo dejes flotante.

**VCC y niveles de señal:** la tensión admisible depende de la placa concreta de cada módulo. El Uno trabaja con señales de 5 V; el BME280 y las tarjetas SD usan lógica de 3,3 V. Comprueba si los módulos incorporan regulador y adaptación de niveles antes de conectarlos al Uno. Un regulador de alimentación por sí solo no adapta las señales. Usa adaptación de niveles si los módulos no toleran 5 V y alimenta VCC conforme a la documentación de cada placa. Confirma que la etiqueta `SDK` de la SD corresponde al reloj SPI.

En la MMC/SD CARD PROTO HW REV. 1.00 del proyecto, las etiquetas SDO y SDI se refieren al microcontrolador. El usuario ha confirmado que SDO→D11 y SDI→D12 corrige el fallo de inicialización `SdError: 0x1,0x0`. La placa se utiliza con alimentación de 5 V y sus cuatro puentes de selección de tensión en 5 V. Otras placas pueden nombrar las señales desde el lado de la tarjeta: comprueba la revisión antes de reutilizar este cableado.

Referencia del pinout: https://content.arduino.cc/assets/A000066-full-pinout.pdf

## Compilar y cargar

Abre esta carpeta con PlatformIO IDE en VS Code o usa PlatformIO Core:

```sh
pio run
pio run --target upload
pio device monitor
```

El monitor serie utiliza 9600 baudios. Las librerías se descargan automáticamente desde `platformio.ini`. Si tienes varios dispositivos USB, especifica `upload_port` y `monitor_port` en ese archivo.

## Registro CSV y tarjeta

Se usa SdFat en modo **exFAT** (`SdExFat` y `SDFAT_FILE_TYPE=2`), para mantener el formato de tu tarjeta de 2 GB. Este firmware no monta FAT16/FAT32 ni formatea tarjetas.

El archivo se crea automáticamente en la raíz al cumplirse el primer minuto. La cabecera se escribe únicamente cuando el archivo está vacío y los siguientes registros se añaden al final:

```csv
fecha,temperatura_C,humedad_pct,presion_hPa
2026-10-14T21:16:00,23.45,48.72,1013.25
2026-10-14T21:17:00,23.48,48.69,1013.22
```

Este ejemplo es ilustrativo. Se usan coma como separador y punto decimal. La presión está en hPa y corresponde a la presión local medida, sin corrección al nivel del mar. Cada fila contiene una lectura puntual; no se calcula una media del minuto. El archivo se sincroniza y cierra después de cada registro.

La pantalla muestra temperatura, humedad, presión y estado de la SD. Si falta la SD, la medición continúa y se reintenta montarla en el siguiente registro válido. Si falla el sensor, se muestra el error y se omite ese registro. Las muestras perdidas no se almacenan ni se recuperan posteriormente. No extraigas la tarjeta durante el funcionamiento; apaga el dispositivo antes de retirarla.

Documentación de exFAT en SdFat: https://github.com/greiman/SdFat

## Diagnóstico de la SD

La conexión SPI se configura a 1 MHz para facilitar el funcionamiento con cableado de prototipos. CS (D10), activo en LOW, se configura como salida y comienza en HIGH.

La OLED distingue las etapas del fallo. El monitor serie a 9600 baudios muestra el mismo mensaje y, cuando existe, el código de SdFat (`SdError: 0X...,0X...`):

| Mensaje | Etapa que falla | Comprobación |
| --- | --- | --- |
| `SD SPI ERROR` | Inicialización de la comunicación con la tarjeta | Tarjeta insertada, alimentación, GND común, CS→D10, SDO→D11, SDI→D12 y SCK/SDK→D13 para la MikroE HW REV. 1.00. |
| `SD exFAT ERROR` | Comunicación inicial correcta, pero no se monta el volumen | Confirmar formato exFAT y revisar la partición en el ordenador; también puede haber errores de lectura. |
| `SD FILE ERROR` | Apertura o creación de `DATOS.CSV` | Revisar archivo, permisos y estado del volumen. |
| `SD WRITE ERROR` | Escritura, sincronización o cierre | Espacio disponible, protección contra escritura y conexión estable. |

Abre `pio device monitor` y pulsa RESET en el Uno para capturar los mensajes de arranque. Conserva el código completo del error para identificar la causa antes de cambiar el formato o el cableado. El firmware no formatea la tarjeta.

## Fecha provisional

Como referencia temporal se usa **la fecha y hora de compilación** (`__DATE__` y `__TIME__`), que avanza mediante `millis()` con RTClib. No requiere un RTC externo.

Cada reinicio vuelve a esa misma fecha y hora; el tiempo apagado no se cuenta. El reloj tiene la precisión del oscilador del Uno y no ajusta zona horaria ni cambios de horario. La fecha de compilación depende del reloj y zona horaria del entorno de compilación. Los registros de distintos arranques pueden tener fechas repetidas. Para fechas reales persistentes se necesitaría un RTC con batería o sincronización externa.

## Organización

- `src/main.cpp`: lectura, OLED, reloj provisional y CSV.
- `platformio.ini`: placa, dependencias y soporte exFAT.
- `include/`: cabeceras compartidas.
- `lib/`: librerías propias.
- `test/`: pruebas futuras.

La pantalla usa U8x8 en modo texto sin framebuffer para ahorrar RAM. Los intervalos están definidos en `kSampleIntervalMs` y `kLogIntervalMs`; las operaciones I2C y SD son síncronas, por lo que una tarjeta lenta o un fallo de bus puede retrasar una actualización.

## Comprobación en hardware

Compilación verificada para Uno: 30 846 bytes de flash (95,6 %) y 1396 bytes de RAM estática (68,2 %). La pila y las asignaciones dinámicas usan parte de la RAM restante. Las versiones directas de librerías y plataforma están fijadas para conservar esta configuración. La OLED y el BME280 han sido comprobados por el usuario; la escritura en la tarjeta sigue pendiente de verificar.

1. Carga el firmware con los módulos conectados y una tarjeta exFAT insertada.
2. Comprueba que la OLED muestra valores que cambian cada segundo y `SD OK`.
3. Tras al menos dos minutos, comprueba en el monitor serie dos mensajes de muestra guardada.
4. Apaga la placa y abre `DATOS.CSV` en el ordenador: una cabecera y dos filas, con aproximadamente 60 segundos entre fechas.
5. Reinicia y comprueba que se añaden filas sin repetir la cabecera, teniendo presente el reinicio de la fecha provisional.
