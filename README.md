# Sistemas_Digitales

Proyecto de Sistemas Digitales: **alarma con sensor PIR, teclado matricial y LCD**.

El sistema detecta movimiento con un sensor PIR HC-SR501 y activa un LED rojo y un buzzer intermitente. La alarma queda enclavada hasta que se ingresa la clave `0000` por un teclado matricial 4×4. Un LCD 16×2 con módulo I2C muestra el estado y la cuenta regresiva de rearme. El firmware usa `millis()` y una máquina de estados (calentamiento, armado, alarma y rearme) sin bloquear el `loop()`.

## Contenido

- `Alarma_PIR_Teclado/` — firmware final de la alarma.
- `Manuales/MANUAL_DEL_PROGRAMADOR.md` y `.pdf` — documentación completa del proyecto, incluidos los cuatro problemas disparadores (sección 7).
- `Teoria/` — material de la consigna y lista de componentes.

## Compilación

Abrir el sketch en el IDE de Arduino, seleccionar la placa **Arduino Uno** y compilar. Requiere las librerías `Keypad` y `LiquidCrystal I2C`.
