#include <Arduino.h>

// Problema disparador 2
// Modificar el dispositivo anterior cambiando el pulsador A por el sensor
// de movimiento PIR501.
// Nota: el LED se debe encender cuando el PIR detecte movimiento y se debe
// apagar al presionar el pulsador B.

const int PIN_PIR        = 2;  // Salida digital del PIR HC-SR501
const int PIN_PULSADOR_B = 3;  // Pulsador B: apaga el LED
const int PIN_LED        = 13;

bool ledEncendido = false;

void setup() {
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_PULSADOR_B, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
}

void loop() {
  bool movimiento = (digitalRead(PIN_PIR) == HIGH);
  bool pulsadorB  = (digitalRead(PIN_PULSADOR_B) == LOW);

  if (movimiento) {
    ledEncendido = true;
  }
  if (pulsadorB) {
    ledEncendido = false;
  }

  digitalWrite(PIN_LED, ledEncendido ? HIGH : LOW);
}
