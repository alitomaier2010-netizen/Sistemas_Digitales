/*
  ============================================================
  Alarma con sensor de movimiento PIR y clave de desactivacion
  ============================================================
  Placa: Arduino Uno R3
  Buzzer: pasivo (se maneja con tone())
  Clave de fabrica: 0000

  Funcionamiento:
    - Reposo (sin deteccion) .... LED VERDE encendido.
    - Detecta movimiento ........ LED VERDE apagado, LED ROJO
                                  encendido y buzzer intermitente.
    - La alarma queda enclavada hasta que se ingresa la clave
      correcta (0000) por el teclado 4x4.
    - Al desactivar, espera 10 s y que el PIR vuelva a reposo
      antes de volver a armarse.
    - Al encender, el PIR calienta 60 s (LED verde parpadeando).

  Conexionado:
    PIR HC-SR501  OUT ....... D2
    LED verde     anodo ..... D3  (+ resistencia 330 ohm a GND)
    LED rojo      anodo ..... D4  (+ resistencia 330 ohm a GND)
    Buzzer pasivo positivo .. D5  (negativo a GND)
    Teclado 4x4   filas ..... D6, D7, D8, D9
    Teclado 4x4   columnas .. A0, A1, A2, A3
    Llave de 2 puntos ....... en serie con la alimentacion externa
                              (no ocupa pin del Arduino)

  Libreria necesaria: Keypad (Gestor de librerias del IDE)
  ============================================================
*/

#include <Keypad.h>

// -------------------- Pines de entrada / salida --------------------
const uint8_t PIR_PIN    = 2;
const uint8_t LED_GREEN  = 3;
const uint8_t LED_RED    = 4;
const uint8_t BUZZER_PIN = 5;

// -------------------- Teclado 4x4 --------------------
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {6, 7, 8, 9};       // R1 a R4
byte colPins[COLS] = {A0, A1, A2, A3};   // C1 a C4

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// -------------------- Clave --------------------
const char PASSWORD[] = "0000";
const uint8_t PASSWORD_LEN = 4;

// -------------------- Tiempos --------------------
const unsigned long WARMUP_MS      = 60000UL;  // calentamiento del PIR
const unsigned long REARM_MS       = 10000UL;  // espera antes de rearmar
const unsigned long PIR_SAMPLE_MS  = 50UL;     // muestreo del PIR
const uint8_t PIR_STABLE_SAMPLES   = 3;        // 3 x 50 ms = 150 ms
const unsigned int BUZZER_FREQ     = 2000;     // Hz
const unsigned long BUZZER_ON_MS   = 250UL;    // pitido encendido
const unsigned long BUZZER_OFF_MS  = 350UL;    // pausa entre pitidos

// -------------------- Estados --------------------
enum State { WARMUP, ARMED, ALARM, REARM };
State state = WARMUP;
unsigned long stateSince = 0;

// -------------------- Variables --------------------
char input[PASSWORD_LEN + 1];
uint8_t inputLen = 0;

bool pirActive     = false;
bool lastPirRaw    = LOW;
bool stablePir     = LOW;
uint8_t pirCount   = 0;
unsigned long lastPirSample = 0;

bool buzzerOn      = false;
unsigned long buzzerPhase = 0;

// -------------------- Prototipos --------------------
void enterState(State newState);
void updatePIR();
void handleKey(char key);
void checkPassword();
void updateStateMachine();
void updateLeds();
void updateBuzzer();
void stopBuzzer();
void clearInput();

// ============================================================
// setup
// ============================================================
void setup() {
  Serial.begin(9600);

  pinMode(PIR_PIN, INPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  keypad.setDebounceTime(20);

  Serial.println(F("=== Alarma PIR + teclado (Arduino Uno) ==="));
  Serial.println(F("Clave de fabrica: 0000"));

  enterState(WARMUP);
}

// ============================================================
// loop principal (no usa delay)
// ============================================================
void loop() {
  updatePIR();

  char key = keypad.getKey();
  if (key) handleKey(key);

  updateStateMachine();
  updateLeds();
  updateBuzzer();
}

// ============================================================
// Cambio de estado
// ============================================================
void enterState(State newState) {
  state = newState;
  stateSince = millis();
  clearInput();

  switch (newState) {
    case WARMUP:
      stopBuzzer();
      Serial.println(F("Estado: CALENTAMIENTO (60 s)"));
      break;

    case ARMED:
      stopBuzzer();
      Serial.println(F("Estado: ARMADO (vigilando)"));
      break;

    case ALARM:
      Serial.println(F("Estado: ALARMA (movimiento detectado)"));
      break;

    case REARM:
      stopBuzzer();
      Serial.println(F("Estado: REARMANDO (10 s)"));
      break;
  }
}

// ============================================================
// Lectura del PIR con antirrebote (150 ms)
// ============================================================
void updatePIR() {
  unsigned long now = millis();
  if (now - lastPirSample < PIR_SAMPLE_MS) return;
  lastPirSample = now;

  bool raw = digitalRead(PIR_PIN);

  // Si cambio la lectura, reinicia el contador
  if (raw != lastPirRaw) {
    lastPirRaw = raw;
    pirCount = 0;
    return;
  }

  if (pirCount < PIR_STABLE_SAMPLES) {
    pirCount++;
    if (pirCount >= PIR_STABLE_SAMPLES) {
      stablePir = raw;
      if (stablePir != pirActive) {
        pirActive = stablePir;
        Serial.println(pirActive ? F("PIR: movimiento") : F("PIR: reposo"));
      }
    }
  }
}

// ============================================================
// Teclado y clave (solo se usa en estado ALARM)
// ============================================================
void handleKey(char key) {
  if (state != ALARM) return;

  bool isDigit   = (key >= '0' && key <= '9');
  bool isControl = (key == '*' || key == '#');
  if (!isDigit && !isControl) return;

  Serial.print(F("Tecla: "));
  Serial.println(key);

  if (key == '*') {          // borrar lo ingresado
    clearInput();
    return;
  }

  if (key == '#') {          // confirmar manualmente
    if (inputLen == PASSWORD_LEN) checkPassword();
    return;
  }

  if (inputLen < PASSWORD_LEN) {
    input[inputLen++] = key;
    input[inputLen] = '\0';

    // Al cuarto digito se valida solo
    if (inputLen == PASSWORD_LEN) checkPassword();
  }
}

void checkPassword() {
  if (strcmp(input, PASSWORD) == 0) {
    Serial.println(F("Clave correcta: alarma desactivada"));
    enterState(REARM);
  } else {
    Serial.println(F("Clave incorrecta: reintente"));
    clearInput();   // sigue sonando la alarma
  }
}

// ============================================================
// Maquina de estados
// ============================================================
void updateStateMachine() {
  unsigned long elapsed = millis() - stateSince;

  switch (state) {
    case WARMUP:
      if (elapsed >= WARMUP_MS) {
        Serial.println(F("PIR estabilizado"));
        enterState(ARMED);
      }
      break;

    case ARMED:
      if (pirActive) enterState(ALARM);
      break;

    case ALARM:
      // Queda enclavada: solo sale con la clave correcta
      break;

    case REARM:
      if (elapsed >= REARM_MS && !pirActive) enterState(ARMED);
      break;
  }
}

// ============================================================
// Salidas: LEDs
// ============================================================
void updateLeds() {
  bool green = false;
  bool red   = false;

  switch (state) {
    case WARMUP:
      green = ((millis() - stateSince) / 500UL) % 2UL == 0UL; // parpadea
      break;

    case ARMED:
      green = true;   // reposo: verde fijo
      break;

    case ALARM:
      red = true;     // alarma: rojo fijo, verde apagado
      break;

    case REARM:
      green = true;
      break;
  }

  digitalWrite(LED_GREEN, green ? HIGH : LOW);
  digitalWrite(LED_RED, red ? HIGH : LOW);
}

// ============================================================
// Salidas: buzzer pasivo intermitente
// ============================================================
void updateBuzzer() {
  if (state != ALARM) {
    stopBuzzer();
    return;
  }

  unsigned long now = millis();

  if (buzzerOn) {
    if (now - buzzerPhase >= BUZZER_ON_MS) {
      stopBuzzer();
      buzzerPhase = now;
    }
  } else {
    if (now - buzzerPhase >= BUZZER_OFF_MS) {
      tone(BUZZER_PIN, BUZZER_FREQ);
      buzzerOn = true;
      buzzerPhase = now;
    }
  }
}

void stopBuzzer() {
  if (buzzerOn) {
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOn = false;
  }
}

void clearInput() {
  inputLen = 0;
  input[0] = '\0';
}
