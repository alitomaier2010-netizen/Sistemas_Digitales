#include <Arduino.h>

// Problema disparador 1
// Diseñar un dispositivo que permita encender un LED con un pulsador A
// y apagar dicho LED con otro pulsador B.

const int PIN_PULSADOR_A = 2; // Pulsador A: enciende el LED
const int PIN_PULSADOR_B = 3; // Pulsador B: apaga el LED
const int PIN_LED        = 13;

bool ledEncendido = false;

void setup() {
  pinMode(PIN_PULSADOR_A, INPUT_PULLUP);
  pinMode(PIN_PULSADOR_B, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
}

void loop() {
  // Con INPUT_PULLUP: presionado = LOW
  bool pulsadorA = (digitalRead(PIN_PULSADOR_A) == LOW);
  bool pulsadorB = (digitalRead(PIN_PULSADOR_B) == LOW);

  if (pulsadorA) {
    ledEncendido = true;
  }
  if (pulsadorB) {
    ledEncendido = false;
  }

  digitalWrite(PIN_LED, ledEncendido ? HIGH : LOW);
}
