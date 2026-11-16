#include <Arduino.h>

// ============================================================
//  PRUEBA DEL SENSOR PIR (solo el sensor)
//  - LED verde (D0) encendido mientras el PIR detecta movimiento
//  - Muestra por Serial el valor de A0 (0..1024) y cada cambio
// ============================================================

const int pinVERDE = 16; // D0
const int pinPIR   = A0; // salida del PIR
const int UMBRAL   = 512;

bool anterior = false;
unsigned long tUltimo = 0;

void setup() {
  pinMode(pinVERDE, OUTPUT);
  digitalWrite(pinVERDE, LOW);
  Serial.begin(115200);
  delay(300);
  Serial.println("\n\n=== PRUEBA DEL SENSOR PIR ===");
  Serial.println("El PIR tarda ~30-60 s en estabilizarse al encender.");
  Serial.println("LED verde = movimiento detectado");
}

void loop() {
  int valor = analogRead(pinPIR);
  bool activo = valor > UMBRAL;
  digitalWrite(pinVERDE, activo ? HIGH : LOW);

  if (activo != anterior) {
    anterior = activo;
    Serial.print(activo ? ">>> MOVIMIENTO  (A0 = " : "--- sin movimiento  (A0 = ");
    Serial.print(valor);
    Serial.println(")");
  }

  if (millis() - tUltimo >= 1000) {   // valor crudo cada 1 s
    tUltimo = millis();
    Serial.print("A0 = ");
    Serial.println(valor);
  }
  delay(20);
}
