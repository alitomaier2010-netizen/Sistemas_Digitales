#include <Arduino.h>
#include <Keypad.h>

// Prueba de hardware para Arduino Uno (sin logica de alarma)
//  - LED verde (D11) y LED rojo + zumbador (D12) alternan cada segundo
//  - Imprime el estado del PIR (D2) y cada tecla pulsada

const byte pinPIR = 2, pinVERDE = 11, pinALARMA = 12;

char teclas[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte pinesFilas[4] = {3, 4, 5, 6};
byte pinesColumnas[4] = {7, 8, 9, 10};
Keypad teclado(makeKeymap(teclas), pinesFilas, pinesColumnas, 4, 4);

void setup() {
  pinMode(pinPIR, INPUT);
  pinMode(pinVERDE, OUTPUT);
  pinMode(pinALARMA, OUTPUT);
  Serial.begin(115200);
  Serial.println(F("=== PRUEBA DE HARDWARE ==="));
}

void loop() {
  static unsigned long t = 0;
  static byte paso = 0;
  if (millis() - t >= 1000) {
    t = millis();
    paso = (paso + 1) % 4;
    noTone(pinALARMA);
    digitalWrite(pinVERDE, paso == 0);
    if (paso == 1) digitalWrite(pinALARMA, HIGH);   // LED rojo fijo
    else if (paso == 2) tone(pinALARMA, 2000);      // zumbador pasivo
    else digitalWrite(pinALARMA, LOW);
    Serial.print(F("Paso "));
    Serial.print(paso);
    Serial.print(F(" | PIR (D2) = "));
    Serial.println(digitalRead(pinPIR));
  }
  char k = teclado.getKey();
  if (k) {
    Serial.print(F("Tecla: "));
    Serial.println(k);
  }
}
