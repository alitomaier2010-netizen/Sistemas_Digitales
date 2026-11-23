#include <Arduino.h>

// Prueba del sensor PIR (HC-SR501) en Arduino Uno.
// Imprime una vez por segundo el valor de D2: 1 = movimiento, 0 = quieto.
// Subir con:  pio run -e prueba_pir -t upload

const byte pinPIR = 2;

void setup() {
  pinMode(pinPIR, INPUT);
  Serial.begin(115200);
  Serial.println(F("=== PRUEBA DEL SENSOR PIR ==="));
  Serial.println(F("Espere 1 minuto a que el sensor se acomode."));
}

void loop() {
  Serial.print(F("PIR (D2) = "));
  Serial.println(digitalRead(pinPIR));
  delay(1000);
}
