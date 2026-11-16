#include <Arduino.h>
#include <Keypad.h>

// Problema disparador 3
// Realizar un programa que permita visualizar en el Monitor Serie de Arduino
// los números y caracteres de un teclado matricial de 4x4.

const byte FILAS = 4;
const byte COLUMNAS = 4;

char teclas[FILAS][COLUMNAS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Ajustar estos pines según el conexionado del teclado
byte pinesFilas[FILAS]       = {9, 8, 7, 6};    // R1, R2, R3, R4
byte pinesColumnas[COLUMNAS] = {5, 4, 3, 2};    // C1, C2, C3, C4

Keypad teclado = Keypad(makeKeymap(teclas), pinesFilas, pinesColumnas, FILAS, COLUMNAS);

void setup() {
  Serial.begin(9600);
  Serial.println("Teclado matricial 4x4 - presione una tecla");
}

void loop() {
  char tecla = teclado.getKey();
  if (tecla) {
    Serial.print("Tecla presionada: ");
    Serial.println(tecla);
  }
}
