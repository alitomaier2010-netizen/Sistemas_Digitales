#include <Arduino.h>
#include <Keypad.h>

// ============================================================
//  ALARMA PIR + TECLADO 4x4 (ADAPTADO PARA ESP8266MOD)
//  A = activar / detener sonido     B = desactivar
//  C = cambiar clave                * = borrar     # = enter
// ============================================================

// Pines seguros para ESP8266 (NodeMCU / ESP-12E)
const int pinVERDE  = 16; // D0: LED verde, encendido mientras el PIR NO detecta nada
const int pinALARMA = 15; // D8: LED rojo + zumbador (en paralelo), encendidos al detectar
const int pinPIR  = A0;  // Entrada analogica (3.3V -> ~1024)

const byte FILAS = 4, COLUMNAS = 4;
char teclas[FILAS][COLUMNAS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// D1(5), D2(4), D5(14), D6(12)
byte pinesFilas[FILAS] = {5, 4, 14, 12};
// D7(13), D3(0), D4(2), RX(3)
byte pinesColumnas[COLUMNAS] = {13, 0, 2, 3};

Keypad teclado(makeKeymap(teclas), pinesFilas, pinesColumnas, FILAS, COLUMNAS);

const unsigned long CALENTAMIENTO = 10000UL;  // 10 s del PIR
const byte MIN_CLAVE = 4, MAX_CLAVE = 8;

enum Estado {
  DESARMADA, ARMANDO, CALENTANDO, ARMADA, SONANDO,
  CLAVE_DETENER, CLAVE_DESARMAR, CLAVE_VIEJA, CLAVE_NUEVA
};
const char* NOMBRE_ESTADO[] = {
  "DESARMADA", "ESPERANDO CLAVE PARA ARMAR", "CALENTANDO PIR", "ARMADA",
  "ALARMA SONANDO", "ESPERANDO CLAVE PARA DETENER",
  "ESPERANDO CLAVE PARA DESARMAR", "CAMBIO: CLAVE ACTUAL",
  "CAMBIO: CLAVE NUEVA"
};

Estado estado = DESARMADA;
Estado estadoRetorno = DESARMADA;   // a donde volver tras cambiar la clave
char clave[MAX_CLAVE + 1] = "1234";
char entrada[MAX_CLAVE + 1] = "";
byte largo = 0;
unsigned long tCalentamiento = 0;
int ultimoSegundo = -1;
bool sonando = false;         // el zumbador esta encendido
bool pirBloqueado = false;    // espera a que el PIR baje antes de disparar otra vez

// ---------------- Utilidades ----------------
void limpiar() {
  entrada[0] = '\0';
  largo = 0;
}

bool claveCorrecta() {
  return strcmp(entrada, clave) == 0;
}

bool pideClave() {
  return estado == ARMANDO || estado == CLAVE_DETENER ||
         estado == CLAVE_DESARMAR || estado == CLAVE_VIEJA ||
         estado == CLAVE_NUEVA;
}

void mostrarEstado() {
  Serial.print("   Estado: ");
  Serial.println(NOMBRE_ESTADO[estado]);
}

bool editarClave(char t) {
  if (t >= '0' && t <= '9') {
    if (largo >= MAX_CLAVE) {
      Serial.println("   (maximo 8 digitos)");
      return true;
    }
    entrada[largo++] = t;
    entrada[largo] = '\0';
  } else if (t == '*') {
    if (largo == 0) {
      Serial.println("   (nada que borrar)");
      return true;
    }
    entrada[--largo] = '\0';
  } else {
    return false;
  }
  Serial.print("   Clave: [");
  Serial.print(entrada);
  Serial.println("]");
  return true;
}

// ---------------- Sonido ----------------
// El LED rojo y el zumbador (pasivo) comparten D8:
//  - fase de pitido: tone() -> el zumbador suena y el LED se ve encendido
//  - fase de silencio: D8 en HIGH fijo -> el zumbador pasivo calla
//    pero el LED rojo sigue encendido
// Resultado: LED rojo encendido fijo + sonido intermitente.
const unsigned int FRECUENCIA = 2000;  // Hz
const unsigned long T_PITIDO   = 300;  // ms sonando
const unsigned long T_SILENCIO = 200;  // ms en silencio
unsigned long tSonido = 0;
bool fasePitido = false;

void actualizarSonido() {
  if (!sonando) return;
  unsigned long fase = (millis() - tSonido) % (T_PITIDO + T_SILENCIO);
  bool pitar = fase < T_PITIDO;
  if (pitar == fasePitido) return;   // sin cambio de fase
  fasePitido = pitar;
  if (pitar) {
    tone(pinALARMA, FRECUENCIA);
  } else {
    noTone(pinALARMA);
    digitalWrite(pinALARMA, HIGH);
  }
}

void apagarSonido() {
  sonando = false;
  fasePitido = false;
  noTone(pinALARMA);
  digitalWrite(pinALARMA, LOW);
  pirBloqueado = true;   // ignorar el movimiento actual hasta que el PIR baje
}

void dispararAlarma() {
  sonando = true;
  estado = SONANDO;
  tSonido = millis();
  fasePitido = false;
  digitalWrite(pinVERDE, LOW);
  digitalWrite(pinALARMA, HIGH);
  actualizarSonido();
  Serial.println("\n!!! MOVIMIENTO DETECTADO !!!");
  Serial.println("Escriba la clave y # para apagar la alarma.");
  Serial.println("(A + clave + # = callar y seguir armada)");
}

void iniciarCalentamiento() {
  limpiar();
  estado = CALENTANDO;
  ultimoSegundo = -1;
  tCalentamiento = millis();
  Serial.println("\nClave correcta. Calentando el PIR (10 s)...");
}

// ---------------- Tecla # ----------------
void confirmar() {
  bool ok = claveCorrecta();

  switch (estado) {
    case ARMANDO:
      if (ok) {
        iniciarCalentamiento();
        return;
      }
      Serial.println("\nClave INCORRECTA. La alarma no se activo.");
      estado = DESARMADA;
      break;

    case CLAVE_DETENER:
      if (ok) {
        apagarSonido();
        estado = ARMADA;
        Serial.println("\nSonido detenido. La alarma sigue ARMADA.");
      } else {
        estado = SONANDO;
        Serial.println("\nClave INCORRECTA. El zumbador sigue sonando.");
      }
      break;

    case CLAVE_DESARMAR:
      if (ok) {
        apagarSonido();
        estado = DESARMADA;
        Serial.println("\n>>> ALARMA DESACTIVADA <<<");
      } else {
        estado = sonando ? SONANDO : ARMADA;
        Serial.println("\nClave INCORRECTA. La alarma sigue activa.");
      }
      break;

    case CLAVE_VIEJA:
      if (ok) {
        estado = CLAVE_NUEVA;
        Serial.println("\nClave actual OK. Ingrese la NUEVA (4 a 8 digitos) y #.");
      } else {
        estado = estadoRetorno;
        Serial.println("\nClave INCORRECTA. No se cambio nada.");
      }
      break;

    case CLAVE_NUEVA:
      if (largo < MIN_CLAVE) {
        Serial.print("\nMinimo ");
        Serial.print(MIN_CLAVE);
        Serial.println(" digitos, siga escribiendo.");
        return;
      }
      strcpy(clave, entrada);
      estado = estadoRetorno;
      Serial.print("\nNueva clave guardada: ");
      Serial.println(clave);
      break;

    default:
      return;
  }
  limpiar();
}

// ---------------- Teclas A, B, C y edicion ----------------
void procesarTecla(char t) {
  Serial.print("\nTecla: ");
  Serial.print(t);

  if (pideClave()) {
    if (editarClave(t)) {
      mostrarEstado();
      return;
    }
    if (t == '#') {
      confirmar();
      mostrarEstado();
      return;
    }
    Serial.println(" (cancelado)");
    if (estado == ARMANDO) estado = DESARMADA;
    else if (estado == CLAVE_DETENER) estado = SONANDO;
    else if (estado == CLAVE_DESARMAR) estado = sonando ? SONANDO : ARMADA;
    else estado = estadoRetorno;
    limpiar();
    mostrarEstado();
    return;
  }

  if (t == 'C' && (estado == DESARMADA || estado == ARMADA)) {
    estadoRetorno = estado;
    limpiar();
    estado = CLAVE_VIEJA;
    Serial.println("\nIngrese la clave ACTUAL y #.");
    mostrarEstado();
    return;
  }

  switch (estado) {
    case DESARMADA:
      if (t == 'A') {
        limpiar();
        estado = ARMANDO;
        Serial.println("\nIngrese la clave y # para activar.");
      } else if (t == 'B') {
        Serial.println("\nLa alarma ya esta desactivada.");
      } else {
        Serial.println("\nUse A (activar), B (desactivar) o C (cambiar clave).");
      }
      break;

    case ARMADA:
      if (t == 'B') {
        limpiar();
        estado = CLAVE_DESARMAR;
        Serial.println("\nIngrese la clave y # para desactivar.");
      } else if (t == 'A') {
        Serial.println("\nLa alarma ya esta armada.");
      } else {
        Serial.println("\nUse B para desactivar o C para cambiar la clave.");
      }
      break;

    case SONANDO:
      if (t == 'A') {
        limpiar();
        estado = CLAVE_DETENER;
        Serial.println("\nIngrese la clave y # para detener el sonido.");
      } else if (t == 'B') {
        limpiar();
        estado = CLAVE_DESARMAR;
        Serial.println("\nIngrese la clave y # para desactivar.");
      } else if (t >= '0' && t <= '9') {
        // escribir la clave directamente: al confirmar con # se desactiva
        limpiar();
        estado = CLAVE_DESARMAR;
        editarClave(t);
      } else {
        Serial.println("\nLa alarma esta sonando. Escriba la clave y #.");
      }
      break;

    case CALENTANDO:
      Serial.println(" (sensor calentando, espere)");
      break;

    default:
      break;
  }
  mostrarEstado();
}

// ---------------- setup / loop ----------------
void setup() {
  pinMode(pinVERDE, OUTPUT);
  pinMode(pinALARMA, OUTPUT);
  digitalWrite(pinVERDE, HIGH);
  digitalWrite(pinALARMA, LOW);

  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== ALARMA PIR + TECLADO 4x4 (ESP8266) ===");
  Serial.println("A activar | B desactivar | C cambiar clave");
  Serial.println("* borrar | # enter | Clave inicial: 1234");
}

void loop() {
  // Calentamiento del PIR sin bloquear el teclado
  if (estado == CALENTANDO) {
    unsigned long t = millis() - tCalentamiento;
    if (t >= CALENTAMIENTO) {
      estado = ARMADA;
      Serial.println("\n>>> ALARMA ARMADA <<<");
    } else {
      int s = (CALENTAMIENTO - t + 999) / 1000;
      if (s != ultimoSegundo) {
        ultimoSegundo = s;
        Serial.print("Calentando... ");
        Serial.print(s);
        Serial.println(" s");
      }
    }
  }

  // Lectura del PIR por A0 (mayor a 512 equivale a HIGH de 3.3V)
  int lecturaPIR = analogRead(pinPIR);
  bool pirActivo = lecturaPIR > 512;

  // DEBUG TEMPORAL: valor de A0 cada 500 ms (borrar cuando funcione)
  static unsigned long tDebug = 0;
  if (millis() - tDebug >= 500) {
    tDebug = millis();
    Serial.print("[A0=");
    Serial.print(lecturaPIR);
    Serial.println("]");
  }
  if (!pirActivo) pirBloqueado = false;
  if (estado == ARMADA && !pirBloqueado && pirActivo) dispararAlarma();

  // Verde = reposo: apagado mientras haya movimiento o la alarma este sonando
  // (aunque el PIR deje de detectar, el verde no vuelve hasta poner la clave)
  digitalWrite(pinVERDE, (pirActivo || sonando) ? LOW : HIGH);
  actualizarSonido();

  char t = teclado.getKey();
  if (t) procesarTecla(t);

  delay(20); // Alimenta el Watchdog del ESP8266
}