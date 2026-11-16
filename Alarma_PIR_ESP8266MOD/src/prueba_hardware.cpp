#include <Arduino.h>
#include <Keypad.h>

// ============================================================
//  PRUEBA DE HARDWARE (sin sensor PIR)
//  1) Secuencia automatica: LED verde, LED rojo + zumbador
//  2) Prueba de teclado: cada tecla se muestra por Serial,
//     hace un pitido corto y parpadea el LED verde.
//     A = enciende/apaga LED verde
//     B = enciende/apaga LED rojo + zumbador
//     D = repite la secuencia automatica
//     # = muestra que teclas faltan probar
// ============================================================

const int pinVERDE  = 16; // D0
const int pinALARMA = 15; // D8: LED rojo + zumbador

const byte FILAS = 4, COLUMNAS = 4;
char teclas[FILAS][COLUMNAS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte pinesFilas[FILAS] = {5, 4, 14, 12};       // D1, D2, D5, D6
byte pinesColumnas[COLUMNAS] = {13, 0, 2, 3};  // D7, D3, D4, RX

Keypad teclado(makeKeymap(teclas), pinesFilas, pinesColumnas, FILAS, COLUMNAS);

bool probada[FILAS][COLUMNAS] = {};
bool verdeOn = false, alarmaOn = false;

void pitido(int ms) {
  digitalWrite(pinALARMA, HIGH);
  delay(ms);
  digitalWrite(pinALARMA, alarmaOn ? HIGH : LOW);
}

void secuencia() {
  Serial.println("\n--- Secuencia automatica ---");

  Serial.println("LED VERDE: 3 parpadeos");
  for (int i = 0; i < 3; i++) {
    digitalWrite(pinVERDE, HIGH); delay(300);
    digitalWrite(pinVERDE, LOW);  delay(300);
  }

  Serial.println("LED ROJO + ZUMBADOR: 3 pitidos");
  for (int i = 0; i < 3; i++) {
    digitalWrite(pinALARMA, HIGH); delay(200);
    digitalWrite(pinALARMA, LOW);  delay(300);
  }

  Serial.println("AMBOS juntos 1 s");
  digitalWrite(pinVERDE, HIGH);
  digitalWrite(pinALARMA, HIGH);
  delay(1000);
  digitalWrite(pinVERDE, verdeOn ? HIGH : LOW);
  digitalWrite(pinALARMA, alarmaOn ? HIGH : LOW);

  Serial.println("--- Fin secuencia ---");
  Serial.println("\nAhora presione TODAS las teclas del teclado (16).");
  Serial.println("A=verde on/off  B=rojo+zumbador on/off  D=repetir  #=faltantes");
}

void mostrarFaltantes() {
  int faltan = 0;
  Serial.print("   Faltan probar: ");
  for (byte f = 0; f < FILAS; f++)
    for (byte c = 0; c < COLUMNAS; c++)
      if (!probada[f][c]) { Serial.print(teclas[f][c]); Serial.print(' '); faltan++; }
  if (faltan == 0) Serial.print("ninguna -> TECLADO OK");
  Serial.println();
}

void marcar(char t) {
  for (byte f = 0; f < FILAS; f++)
    for (byte c = 0; c < COLUMNAS; c++)
      if (teclas[f][c] == t) {
        bool nueva = !probada[f][c];
        probada[f][c] = true;
        Serial.print("Tecla ");
        Serial.print(t);
        Serial.print("  (fila ");
        Serial.print(f + 1);
        Serial.print(" pin GPIO");
        Serial.print(pinesFilas[f]);
        Serial.print(", columna ");
        Serial.print(c + 1);
        Serial.print(" pin GPIO");
        Serial.print(pinesColumnas[c]);
        Serial.println(nueva ? ")  NUEVA" : ")");
      }
}

void setup() {
  pinMode(pinVERDE, OUTPUT);
  pinMode(pinALARMA, OUTPUT);
  digitalWrite(pinVERDE, LOW);
  digitalWrite(pinALARMA, LOW);

  Serial.begin(115200);
  delay(300);
  Serial.println("\n\n=== PRUEBA DE HARDWARE (sin PIR) ===");
  secuencia();
}

void loop() {
  char t = teclado.getKey();
  if (t) {
    marcar(t);

    if (t == 'A') {
      verdeOn = !verdeOn;
      digitalWrite(pinVERDE, verdeOn ? HIGH : LOW);
      Serial.println(verdeOn ? "   LED verde ENCENDIDO" : "   LED verde APAGADO");
    } else if (t == 'B') {
      alarmaOn = !alarmaOn;
      digitalWrite(pinALARMA, alarmaOn ? HIGH : LOW);
      Serial.println(alarmaOn ? "   Rojo + zumbador ENCENDIDO" : "   Rojo + zumbador APAGADO");
    } else if (t == 'D') {
      secuencia();
    } else {
      // feedback: pitido corto + parpadeo verde
      digitalWrite(pinVERDE, !verdeOn);
      pitido(60);
      delay(60);
      digitalWrite(pinVERDE, verdeOn ? HIGH : LOW);
    }
    mostrarFaltantes();
  }
  delay(10);
}
