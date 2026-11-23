#include <Arduino.h>
#include <Keypad.h>

// ============================================================
//  ALARMA PIR + TECLADO 4x4 (ARDUINO UNO)
//  A = activar / detener sonido     B = desactivar
//  C = cambiar clave                * = borrar     # = enter
// ============================================================

// Macros para guardar los textos en Flash (el Uno solo tiene 2 KB de RAM)
#define PL(s) Serial.println(F(s))
#define PP(s) Serial.print(F(s))

// ---------------- Pines ----------------
const byte pinPIR    = 2;   // D2: salida del HC-SR501 (HIGH = movimiento)
const byte pinVERDE  = 11;  // D11: LED verde, encendido mientras el PIR NO detecta nada
const byte pinALARMA = 12;  // D12: LED rojo + zumbador (en paralelo), encendidos al detectar

const byte FILAS = 4, COLUMNAS = 4;
char teclas[FILAS][COLUMNAS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte pinesFilas[FILAS]       = {3, 4, 5, 6};   // D3, D4, D5, D6
byte pinesColumnas[COLUMNAS] = {7, 8, 9, 10};  // D7, D8, D9, D10

Keypad teclado(makeKeymap(teclas), pinesFilas, pinesColumnas, FILAS, COLUMNAS);

const unsigned long CALENTAMIENTO = 60000UL;  // 60 s para calibrar el PIR
const byte MIN_CLAVE = 4, MAX_CLAVE = 8;

enum Estado {
  DESARMADA, ARMANDO, CALENTANDO, ARMADA, SONANDO,
  CLAVE_DETENER, CLAVE_DESARMAR, CLAVE_VIEJA, CLAVE_NUEVA
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
  PP("   Estado: ");
  switch (estado) {
    case DESARMADA:      PL("DESARMADA"); break;
    case ARMANDO:        PL("ESPERANDO CLAVE PARA ARMAR"); break;
    case CALENTANDO:     PL("CALENTANDO PIR"); break;
    case ARMADA:         PL("ARMADA"); break;
    case SONANDO:        PL("ALARMA SONANDO"); break;
    case CLAVE_DETENER:  PL("ESPERANDO CLAVE PARA DETENER"); break;
    case CLAVE_DESARMAR: PL("ESPERANDO CLAVE PARA DESARMAR"); break;
    case CLAVE_VIEJA:    PL("CAMBIO: CLAVE ACTUAL"); break;
    case CLAVE_NUEVA:    PL("CAMBIO: CLAVE NUEVA"); break;
  }
}

bool editarClave(char t) {
  if (t >= '0' && t <= '9') {
    if (largo >= MAX_CLAVE) {
      PL("   (maximo 8 digitos)");
      return true;
    }
    entrada[largo++] = t;
    entrada[largo] = '\0';
  } else if (t == '*') {
    if (largo == 0) {
      PL("   (nada que borrar)");
      return true;
    }
    entrada[--largo] = '\0';
  } else {
    return false;
  }
  PP("   Clave: [");
  Serial.print(entrada);
  PL("]");
  return true;
}

// ---------------- Sonido ----------------
// El LED rojo y el zumbador (pasivo) comparten D12:
//  - fase de pitido: tone() -> el zumbador suena y el LED se ve encendido
//  - fase de silencio: D12 en HIGH fijo -> el zumbador pasivo calla
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
  PL("\n!!! MOVIMIENTO DETECTADO !!!");
  PL("Escriba la clave y # para apagar la alarma.");
  PL("(A + clave + # = callar y seguir armada)");
}

void iniciarCalentamiento() {
  limpiar();
  estado = CALENTANDO;
  ultimoSegundo = -1;
  tCalentamiento = millis();
  PL("\nClave correcta. Calentando el PIR (60 s)...");
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
      PL("\nClave INCORRECTA. La alarma no se activo.");
      estado = DESARMADA;
      break;

    case CLAVE_DETENER:
      if (ok) {
        apagarSonido();
        estado = ARMADA;
        PL("\nSonido detenido. La alarma sigue ARMADA.");
      } else {
        estado = SONANDO;
        PL("\nClave INCORRECTA. El zumbador sigue sonando.");
      }
      break;

    case CLAVE_DESARMAR:
      if (ok) {
        apagarSonido();
        estado = DESARMADA;
        PL("\n>>> ALARMA DESACTIVADA <<<");
      } else {
        estado = sonando ? SONANDO : ARMADA;
        PL("\nClave INCORRECTA. La alarma sigue activa.");
      }
      break;

    case CLAVE_VIEJA:
      if (ok) {
        estado = CLAVE_NUEVA;
        PL("\nClave actual OK. Ingrese la NUEVA (4 a 8 digitos) y #.");
      } else {
        estado = estadoRetorno;
        PL("\nClave INCORRECTA. No se cambio nada.");
      }
      break;

    case CLAVE_NUEVA:
      if (largo < MIN_CLAVE) {
        PP("\nMinimo ");
        Serial.print(MIN_CLAVE);
        PL(" digitos, siga escribiendo.");
        return;
      }
      strcpy(clave, entrada);
      estado = estadoRetorno;
      PP("\nNueva clave guardada: ");
      Serial.println(clave);
      break;

    default:
      return;
  }
  limpiar();
}

// ---------------- Teclas A, B, C y edicion ----------------
void procesarTecla(char t) {
  PP("\nTecla: ");
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
    PL(" (cancelado)");
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
    PL("\nIngrese la clave ACTUAL y #.");
    mostrarEstado();
    return;
  }

  switch (estado) {
    case DESARMADA:
      if (t == 'A') {
        limpiar();
        estado = ARMANDO;
        PL("\nIngrese la clave y # para activar.");
      } else if (t == 'B') {
        PL("\nLa alarma ya esta desactivada.");
      } else {
        PL("\nUse A (activar), B (desactivar) o C (cambiar clave).");
      }
      break;

    case ARMADA:
      if (t == 'B') {
        limpiar();
        estado = CLAVE_DESARMAR;
        PL("\nIngrese la clave y # para desactivar.");
      } else if (t == 'A') {
        PL("\nLa alarma ya esta armada.");
      } else {
        PL("\nUse B para desactivar o C para cambiar la clave.");
      }
      break;

    case SONANDO:
      if (t == 'A') {
        limpiar();
        estado = CLAVE_DETENER;
        PL("\nIngrese la clave y # para detener el sonido.");
      } else if (t == 'B') {
        limpiar();
        estado = CLAVE_DESARMAR;
        PL("\nIngrese la clave y # para desactivar.");
      } else if (t >= '0' && t <= '9') {
        // escribir la clave directamente: al confirmar con # se desactiva
        limpiar();
        estado = CLAVE_DESARMAR;
        editarClave(t);
      } else {
        PL("\nLa alarma esta sonando. Escriba la clave y #.");
      }
      break;

    case CALENTANDO:
      PL(" (sensor calentando, espere)");
      break;

    default:
      break;
  }
  mostrarEstado();
}

// ---------------- setup / loop ----------------
void setup() {
  pinMode(pinPIR, INPUT);
  pinMode(pinVERDE, OUTPUT);
  pinMode(pinALARMA, OUTPUT);
  digitalWrite(pinVERDE, HIGH);
  digitalWrite(pinALARMA, LOW);

  Serial.begin(115200);
  PL("\n=== ALARMA PIR + TECLADO 4x4 (ARDUINO UNO) ===");
  PL("A activar | B desactivar | C cambiar clave");
  PL("* borrar | # enter | Clave inicial: 1234");
}

void loop() {
  // Calentamiento del PIR sin bloquear el teclado
  if (estado == CALENTANDO) {
    unsigned long t = millis() - tCalentamiento;
    if (t >= CALENTAMIENTO) {
      estado = ARMADA;
      PL("\n>>> ALARMA ARMADA <<<");
    } else {
      int s = (CALENTAMIENTO - t + 999) / 1000;
      if (s != ultimoSegundo) {
        ultimoSegundo = s;
        PP("Calentando... ");
        Serial.print(s);
        PL(" s");
      }
    }
  }

  // Lectura digital del PIR (HIGH = movimiento)
  bool pirActivo = digitalRead(pinPIR) == HIGH;

  // Valor de D2 cada 1 s para ver que detecta el sensor
  static unsigned long tDebug = 0;
  if (millis() - tDebug >= 1000) {
    tDebug = millis();
    PP("[D2=");
    Serial.print(pirActivo ? 1 : 0);
    PL("]");
  }
  if (!pirActivo) pirBloqueado = false;
  if (estado == ARMADA && !pirBloqueado && pirActivo) dispararAlarma();

  // Verde = reposo: apagado mientras haya movimiento o la alarma este sonando
  // (aunque el PIR deje de detectar, el verde no vuelve hasta poner la clave)
  digitalWrite(pinVERDE, (pirActivo || sonando) ? LOW : HIGH);
  actualizarSonido();

  char t = teclado.getKey();
  if (t) procesarTecla(t);
}
