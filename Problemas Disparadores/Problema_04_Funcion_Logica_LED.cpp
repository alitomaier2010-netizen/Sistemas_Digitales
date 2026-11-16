#include <Arduino.h>

// Problema disparador 4
// Diseñar un programa que permita visualizar, con un indicador LED, el estado
// lógico de la siguiente función lógica para cada una de las combinaciones
// de entrada.
//
// Función (interpretada del enunciado; el texto extraído del PDF tenía
// caracteres corruptos): Z = B · (A + C)
//   · = AND, + = OR
//
// Nota: en protoboard se arma el circuito con la etapa de conmutación de las
// entradas y la etapa de detección de salida con el LED.

// Entradas de conmutación (usar resistencias/pulsadores o llaves según montaje)
const int PIN_A = 2;
const int PIN_B = 3;
const int PIN_C = 4;
const int PIN_LED_Z = 13; // Indicador del estado lógico de Z

void setup() {
  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);
  pinMode(PIN_C, INPUT_PULLUP);
  pinMode(PIN_LED_Z, OUTPUT);

  Serial.begin(9600);
  Serial.println("Funcion logica: Z = B AND (A OR C)");
  Serial.println("A B C | Z");
}

void loop() {
  // Con INPUT_PULLUP: nivel bajo = activo/lógico 1 (ajustar si el montaje es distinto)
  bool A = (digitalRead(PIN_A) == LOW);
  bool B = (digitalRead(PIN_B) == LOW);
  bool C = (digitalRead(PIN_C) == LOW);

  bool Z = B && (A || C);

  digitalWrite(PIN_LED_Z, Z ? HIGH : LOW);

  Serial.print(A); Serial.print(" ");
  Serial.print(B); Serial.print(" ");
  Serial.print(C); Serial.print(" | ");
  Serial.println(Z ? 1 : 0);

  delay(200);
}
