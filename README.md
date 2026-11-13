# Sistemas_Digitales

Proyecto de Sistemas Digitales I: **alarma con sensor PIR, teclado matricial y clave de desactivación**.

El sistema detecta movimiento con un sensor PIR HC-SR501 y responde encendiendo un LED rojo y un buzzer intermitente. Mientras no hay detección, el LED verde queda encendido. La alarma se enclava y solo se desactiva ingresando la clave `0000` por un teclado matricial 4×4. El firmware usa `millis()` y una máquina de estados (calentamiento, armado, alarma y rearme) sin bloquear el `loop()`.

## Contenido

| Ruta | Descripción |
|---|---|
| `Alarma_PIR_Teclado_Uno/` | **Firmware para Arduino Uno** (versión simple, sin LCD). Es el que se usa en el montaje actual. |
| `Alarma_PIR_Teclado/` | Versión anterior escrita para **ESP8266/NodeMCU**. Se conserva como respaldo; no compilar en un Uno. |
| `CONEXIONADO.md` | Guía de cableado paso a paso, mapa de pines y verificación del teclado. |
| `Manuales/MANUAL_DEL_PROGRAMADOR.md` y `.pdf` | Documentación completa del proyecto, incluidos los cuatro problemas disparadores (sección 7) y la versión completa con LCD (sección 6). |
| `Teoria/` | Material de la consigna y lista de componentes. |

## Pines (Arduino Uno)

| Componente | Pin |
|---|---|
| PIR HC-SR501 `OUT` | D2 |
| LED verde | D3 (con 330 Ω) |
| LED rojo | D4 (con 330 Ω) |
| Buzzer pasivo | D5 |
| Teclado filas R1–R4 | D6, D7, D8, D9 |
| Teclado columnas C1–C4 | A0, A1, A2, A3 |
| LCD I2C (opcional, versión completa) | SDA A4, SCL A5 |

El detalle del conexionado está en [`CONEXIONADO.md`](CONEXIONADO.md).

## Comportamiento

| Estado | LED verde | LED rojo | Buzzer |
|---|---|---|---|
| Calentamiento (60 s) | Parpadea | Apagado | Silencio |
| Reposo | Encendido | Apagado | Silencio |
| Alarma | Apagado | Encendido | Intermitente (2 kHz, 250 ms on / 350 ms off) |
| Rearme (10 s) | Encendido | Apagado | Silencio |

La alarma permanece activa aunque la persona deje de moverse. Solo la clave correcta la desactiva.

## Compilación

1. Abrir el sketch en el Arduino IDE y seleccionar la placa **Arduino Uno**.
2. Instalar la librería **Keypad** desde el Gestor de librerías.
3. La versión simple solo requiere `Keypad`. La versión completa del manual requiere además `LiquidCrystal I2C`.
4. Subir el programa y abrir el monitor serie a **9600 baudios**.
