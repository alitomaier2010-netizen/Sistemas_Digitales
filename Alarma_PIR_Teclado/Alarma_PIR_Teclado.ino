#include <Keypad.h>

// -------------------- Pines ESP8266 --------------------
const uint8_t BUZZER_PIN = D0;
const uint8_t PIR_PIN = D5;
const uint8_t LED_RED = D6;
const uint8_t LED_GREEN = D7;
const uint8_t SWITCH_PIN = D8; // Llave interruptora de palanca

// -------------------- Teclado 4x4 --------------------
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Pines exactos que conectaste en la protoboard
byte rowPins[ROWS] = {3, 1, D3, D4}; // RX, TX, D3, D4
byte colPins[COLS] = {D1, D2, 9, 10}; // D1, D2, S2, S3

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// -------------------- Clave --------------------
const char PASSWORD[] = "0000"; // Tu clave de fábrica
const uint8_t PASSWORD_LEN = 4;

// -------------------- Tiempos --------------------
const unsigned long WARMUP_MS = 60000UL; // 60 segundos de calentamiento del PIR
const unsigned long REARM_MS = 10000UL;  // 10 segundos de rearme
const unsigned long PIR_SAMPLE_MS = 50UL;
const uint8_t PIR_STABLE_SAMPLES = 3;
const unsigned long BUZZER_ON_MS = 250UL;
const unsigned long BUZZER_OFF_MS = 350UL;
const unsigned int BUZZER_FREQ = 2000;
const unsigned long ERROR_MS = 1500UL;
const unsigned long MUTE_TIMEOUT_MS = 15000UL;

// -------------------- Estados --------------------
enum State { OFF, WARMUP, ARMED, TRIGGERED, REARMING };
State state = OFF;
unsigned long stateSince = 0;

// -------------------- Variables --------------------
char input[PASSWORD_LEN + 1];
uint8_t inputLen = 0;

bool pirActive = false;
bool lastPirRaw = LOW;
bool stablePir = LOW;
uint8_t pirCount = 0;
unsigned long lastPirSample = 0;

bool alarmSoundEnabled = false;
bool buzzerOn = false;
unsigned long buzzerPhase = 0;
bool buzzerMuted = false;
unsigned long muteStart = 0;

bool errorActive = false;
unsigned long errorSince = 0;
bool errorBuzzerOn = false;

// -------------------- Prototipos --------------------
void enterState(State newState);
void updatePIR();
void handleKey(char key);
void checkPassword();
void triggerError();
void updateError();
void updateStateMachine();
void updateLeds();
void updateBuzzer();
void startAlarmSound();
void stopAlarmSound();
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
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(BUZZER_PIN, LOW); 

  keypad.setDebounceTime(20);

  Serial.println(F("=== Alarma PIR + Teclado (ESP8266) ==="));
  Serial.println(F("Clave de fabrica: 0000"));
  
  enterState(OFF); // El sistema arranca leyendo si la llave está prendida
}

// ============================================================
// loop principal
// ============================================================
void loop() {
  // Lectura de la llave general (Interruptor de palanca)
  bool sistemaEncendido = (digitalRead(SWITCH_PIN) == LOW);
  
  if (!sistemaEncendido) {
    if (state != OFF) {
      enterState(OFF); // Apagar todo si se baja la palanca
    }
    return; // No procesar nada más si está apagado
  } else {
    if (state == OFF) {
      enterState(WARMUP); // Iniciar ciclo si se sube la palanca
    }
  }

  updatePIR();

  char key = keypad.getKey();
  if (key) handleKey(key);

  updateStateMachine();
  updateError();
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
    case OFF:
      stopAlarmSound();
      buzzerMuted = false;
      errorActive = false;
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, LOW);
      Serial.println(F("Estado: SISTEMA APAGADO (Llave abajo)"));
      break;

    case WARMUP:
      stopAlarmSound();
      buzzerMuted = false;
      errorActive = false;
      Serial.println(F("Estado: CALENTAMIENTO (60 segundos)"));
      break;

    case ARMED:
      stopAlarmSound();
      buzzerMuted = false;
      errorActive = false;
      Serial.println(F("Estado: ARMADO (Vigilando)"));
      break;

    case TRIGGERED:
      buzzerMuted = false;
      errorActive = false;
      startAlarmSound();
      Serial.println(F("Estado: ALARMA SONANDO"));
      break;

    case REARMING:
      stopAlarmSound();
      buzzerMuted = false;
      errorActive = false;
      Serial.println(F("Estado: REARMANDO (10 segundos)"));
      break;
  }
}

// ============================================================
// Lectura del PIR con antirrebote
// ============================================================
void updatePIR() {
  unsigned long now = millis();
  if (now - lastPirSample < PIR_SAMPLE_MS) return;
  lastPirSample = now;

  bool raw = digitalRead(PIR_PIN);

  if (raw != lastPirRaw) {
    lastPirRaw = raw;
    pirCount = 0;
  } else if (pirCount < PIR_STABLE_SAMPLES) {
    pirCount++;
    if (pirCount >= PIR_STABLE_SAMPLES) {
      stablePir = raw;
      if (stablePir != pirActive) {
        pirActive = stablePir;
        if (pirActive) Serial.println(F("PIR: MOVIMIENTO"));
        else Serial.println(F("PIR: REPOSO"));
      }
    }
  }
}

// ============================================================
// Teclado y clave
// ============================================================
void handleKey(char key) {
  if (state != TRIGGERED || errorActive) return;

  bool isDigit = (key >= '0' && key <= '9');
  bool isControl = (key == '*' || key == '#');
  if (!isDigit && !isControl) return;

  unsigned long now = millis();

  // Si tocas el teclado mientras suena la alarma, se silencia 15s
  if (!buzzerMuted) {
    buzzerMuted = true;
    stopAlarmSound();
    Serial.println(F("Buzzer silenciado (15 s) por escribir en teclado"));
  }
  muteStart = now;

  Serial.print(F("Tecla: "));
  Serial.println(key);

  if (key == '*') {
    clearInput(); // Borrar si te equivocaste
    return;
  }

  // Confirmar si toca # o si ya completó los 4 números
  if (key == '#') {
    if (inputLen == PASSWORD_LEN) checkPassword();
    return;
  }

  if (inputLen < PASSWORD_LEN) {
    input[inputLen++] = key;
    input[inputLen] = '\0';
    if (inputLen == PASSWORD_LEN) checkPassword(); // Chequeo automático al cuarto número
  }
}

void checkPassword() {
  if (strcmp(input, PASSWORD) == 0) {
    Serial.println(F("Clave correcta"));
    clearInput();
    enterState(REARMING);
  } else {
    Serial.println(F("Clave incorrecta"));
    clearInput();
    triggerError();
  }
}

void triggerError() {
  stopAlarmSound();
  errorActive = true;
  errorSince = millis();
  errorBuzzerOn = false;
  buzzerMuted = true;
  muteStart = millis();
  Serial.println(F("Error de clave: 3 beeps rápidos"));
}

void updateError() {
  if (!errorActive) return;
  if (millis() - errorSince < ERROR_MS) return;

  errorActive = false;
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);
  errorBuzzerOn = false;
  buzzerMuted = false;
  Serial.println(F("Fin del aviso de error"));

  if (state == TRIGGERED) startAlarmSound(); // Vuelve a sonar la alarma principal
}

// ============================================================
// Máquina de estados
// ============================================================
void updateStateMachine() {
  if (state == OFF) return;

  unsigned long elapsed = millis() - stateSince;

  switch (state) {
    case WARMUP:
      if (elapsed >= WARMUP_MS) {
        Serial.println(F("PIR estabilizado"));
        enterState(ARMED);
      }
      break;

    case ARMED:
      if (pirActive) enterState(TRIGGERED);
      break;

    case TRIGGERED:
      break;

    case REARMING:
      if (elapsed >= REARM_MS && !pirActive) enterState(ARMED);
      break;
  }
}

// ============================================================
// Salidas: LEDs y buzzer
// ============================================================
void updateLeds() {
  if (state == OFF) return; 

  bool green = false;
  bool red = false;

  switch (state) {
    case WARMUP:
      green = ((millis() - stateSince) / 500UL) % 2UL == 0UL; // Parpadeo verde
      break;
    case ARMED:
      green = true; // Verde fijo
      break;
    case TRIGGERED:
      red = true; // Rojo fijo
      break;
    case REARMING:
      green = true; // Verde fijo
      break;
  }

  if (errorActive) {
    green = false;
    red = ((millis() - errorSince) % 300UL) < 150UL; // Parpadeo rojo rápido si hay error
  }

  digitalWrite(LED_GREEN, green ? HIGH : LOW);
  digitalWrite(LED_RED, red ? HIGH : LOW);
}

void startAlarmSound() {
  alarmSoundEnabled = true;
  buzzerOn = true;
  tone(BUZZER_PIN, BUZZER_FREQ);
  buzzerPhase = millis();
}

void stopAlarmSound() {
  alarmSoundEnabled = false;
  if (buzzerOn) {
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW); // Seguro para apagar en ESP8266
    buzzerOn = false;
  }
}

void updateBuzzer() {
  if (state == OFF) return;

  unsigned long now = millis();

  // Beeps de error
  if (errorActive) {
    bool on = ((now - errorSince) % 500UL) < 150UL;
    if (on && !errorBuzzerOn) {
      tone(BUZZER_PIN, 1200);
      errorBuzzerOn = true;
    } else if (!on && errorBuzzerOn) {
      noTone(BUZZER_PIN);
      digitalWrite(BUZZER_PIN, LOW);
      errorBuzzerOn = false;
    }
    return;
  }

  // Reanudar alarma si se acaba el tiempo de silencio (15s)
  if (state == TRIGGERED && buzzerMuted && !alarmSoundEnabled &&
      (now - muteStart >= MUTE_TIMEOUT_MS)) {
    buzzerMuted = false;
    startAlarmSound();
  }

  if (!alarmSoundEnabled) return;

  // Intermitencia de la alarma principal
  if (buzzerOn) {
    if (now - buzzerPhase >= BUZZER_ON_MS) {
      noTone(BUZZER_PIN);
      digitalWrite(BUZZER_PIN, LOW);
      buzzerOn = false;
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

void clearInput() {
  inputLen = 0;
  input[0] = '\0';
}