/*
  ============================================================
  Alarma con sensor de movimiento y clave de desactivación
  Proyecto Integrador - Sistemas Digitales I
  Arduino Uno R3 + PIR HC-SR501 + teclado 4x4 + LCD 16x2 I2C
  ============================================================

  Comportamiento:
  - WARMUP: calentamiento del PIR (60 s), LED verde parpadeando.
  - ARMED: reposo, LED verde encendido.
  - TRIGGERED: movimiento, LED rojo y buzzer intermitente.
  - REARMING: clave correcta, LED verde y espera de 10 s.

  Clave de fábrica: 0000
  ============================================================
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <string.h>
#include <stdio.h>

// -------------------- Pines --------------------
const uint8_t PIR_PIN = 2;
const uint8_t LED_GREEN = 3;
const uint8_t LED_RED = 4;
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
byte rowPins[ROWS] = {6, 7, 8, 9};
byte colPins[COLS] = {A0, A1, A2, A3};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// -------------------- LCD 16x2 I2C --------------------
// Si el módulo usa 0x3F, cambiar la dirección.
LiquidCrystal_I2C lcd(0x27, 16, 2);

// -------------------- Clave --------------------
const char PASSWORD[] = "0000";
const uint8_t PASSWORD_LEN = 4;

// -------------------- Tiempos --------------------
const unsigned long WARMUP_MS = 60000UL;
const unsigned long REARM_MS = 10000UL;
const unsigned long PIR_SAMPLE_MS = 50UL;
const uint8_t PIR_STABLE_SAMPLES = 3;
const unsigned long BUZZER_ON_MS = 250UL;
const unsigned long BUZZER_OFF_MS = 350UL;
const unsigned int BUZZER_FREQ = 2000;
const unsigned long ERROR_MS = 1500UL;
const unsigned long MUTE_TIMEOUT_MS = 15000UL;
const unsigned long LCD_REFRESH_MS = 250UL;

// -------------------- Estados --------------------
enum State { WARMUP, ARMED, TRIGGERED, REARMING };
State state = WARMUP;
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

bool lcdDirty = true;
unsigned long lastLcdRefresh = 0;
int lastCountdown = -1;

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
void updateLCD();
void startAlarmSound();
void stopAlarmSound();
void clearInput();
void buildMask(char *out);
void lcdPrintLine(uint8_t row, const char *text);

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
  noTone(BUZZER_PIN);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  keypad.setDebounceTime(20);

  Serial.println(F("=== Alarma PIR + teclado ==="));
  Serial.println(F("Clave de fabrica: 0000"));
  Serial.println(F("Calentando el PIR 60 s..."));

  enterState(WARMUP);
}

// ============================================================
// loop principal
// ============================================================
void loop() {
  updatePIR();

  char key = keypad.getKey();
  if (key) handleKey(key);

  updateStateMachine();
  updateError();
  updateLeds();
  updateBuzzer();
  updateLCD();
}

// ============================================================
// Cambio de estado
// ============================================================
void enterState(State newState) {
  state = newState;
  stateSince = millis();
  lastCountdown = -1;
  lcdDirty = true;
  clearInput();

  switch (newState) {
    case WARMUP:
      stopAlarmSound();
      buzzerMuted = false;
      errorActive = false;
      Serial.println(F("Estado: CALENTAMIENTO"));
      break;

    case ARMED:
      stopAlarmSound();
      buzzerMuted = false;
      errorActive = false;
      Serial.println(F("Estado: ARMADO"));
      break;

    case TRIGGERED:
      buzzerMuted = false;
      errorActive = false;
      startAlarmSound();
      Serial.println(F("Estado: ALARMA"));
      break;

    case REARMING:
      stopAlarmSound();
      buzzerMuted = false;
      errorActive = false;
      Serial.println(F("Estado: REARMANDO"));
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

  if (!buzzerMuted) {
    buzzerMuted = true;
    stopAlarmSound();
    Serial.println(F("Buzzer silenciado (15 s)"));
  }
  muteStart = now;

  Serial.print(F("Tecla: "));
  Serial.println(key);

  if (key == '*') {
    clearInput();
    return;
  }

  if (key == '#') {
    if (inputLen == PASSWORD_LEN) checkPassword();
    return;
  }

  if (inputLen < PASSWORD_LEN) {
    input[inputLen++] = key;
    input[inputLen] = '\0';
    lcdDirty = true;
    if (inputLen == PASSWORD_LEN) checkPassword();
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
  lcdDirty = true;
  Serial.println(F("Error de clave: 3 beeps"));
}

void updateError() {
  if (!errorActive) return;
  if (millis() - errorSince < ERROR_MS) return;

  errorActive = false;
  noTone(BUZZER_PIN);
  errorBuzzerOn = false;
  buzzerMuted = false;
  lcdDirty = true;
  Serial.println(F("Fin del aviso de error"));

  if (state == TRIGGERED) startAlarmSound();
}

// ============================================================
// Máquina de estados
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
// Salidas: LEDs, buzzer y LCD
// ============================================================
void updateLeds() {
  bool green = false;
  bool red = false;

  switch (state) {
    case WARMUP:
      green = ((millis() - stateSince) / 500UL) % 2UL == 0UL;
      break;
    case ARMED:
      green = true;
      break;
    case TRIGGERED:
      red = true;
      break;
    case REARMING:
      green = true;
      break;
  }

  if (errorActive) {
    green = false;
    red = ((millis() - errorSince) % 300UL) < 150UL;
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
    buzzerOn = false;
  }
}

void updateBuzzer() {
  unsigned long now = millis();

  if (errorActive) {
    bool on = ((now - errorSince) % 500UL) < 150UL;
    if (on && !errorBuzzerOn) {
      tone(BUZZER_PIN, 1200);
      errorBuzzerOn = true;
    } else if (!on && errorBuzzerOn) {
      noTone(BUZZER_PIN);
      errorBuzzerOn = false;
    }
    return;
  }

  if (state == TRIGGERED && buzzerMuted && !alarmSoundEnabled &&
      (now - muteStart >= MUTE_TIMEOUT_MS)) {
    buzzerMuted = false;
    startAlarmSound();
  }

  if (!alarmSoundEnabled) return;

  if (buzzerOn) {
    if (now - buzzerPhase >= BUZZER_ON_MS) {
      noTone(BUZZER_PIN);
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
  lcdDirty = true;
}

void buildMask(char *out) {
  for (uint8_t i = 0; i < PASSWORD_LEN; i++) {
    out[i] = (i < inputLen) ? '*' : '_';
  }
  out[PASSWORD_LEN] = '\0';
}

void lcdPrintLine(uint8_t row, const char *text) {
  char buf[17];
  snprintf(buf, sizeof(buf), "%-16s", text);
  lcd.setCursor(0, row);
  lcd.print(buf);
}

void updateLCD() {
  unsigned long now = millis();

  if (!lcdDirty && (now - lastLcdRefresh < LCD_REFRESH_MS)) return;
  lastLcdRefresh = now;

  char line1[17];
  char line2[17];

  switch (state) {
    case WARMUP: {
      int remaining = (int)((WARMUP_MS - (now - stateSince) + 999UL) / 1000UL);
      if (remaining < 0) remaining = 0;
      if (!lcdDirty && remaining == lastCountdown) return;
      lastCountdown = remaining;
      strcpy(line1, "Calentando PIR");
      snprintf(line2, sizeof(line2), "Espere: %d s", remaining);
      break;
    }

    case ARMED:
      if (!lcdDirty) return;
      strcpy(line1, "Sistema armado");
      strcpy(line2, "Sin movimiento");
      break;

    case TRIGGERED:
      if (!lcdDirty) return;
      if (errorActive) {
        strcpy(line1, "Clave incorrecta");
        strcpy(line2, "Intente de nuevo");
      } else {
        char mask[PASSWORD_LEN + 1];
        buildMask(mask);
        strcpy(line1, "Movimiento!");
        snprintf(line2, sizeof(line2), "Clave: %s", mask);
      }
      break;

    case REARMING: {
      int remaining = (int)((REARM_MS - (now - stateSince) + 999UL) / 1000UL);
      if (remaining < 0) remaining = 0;
      if (!lcdDirty && remaining == lastCountdown) return;
      lastCountdown = remaining;
      strcpy(line1, "Alarma OFF");
      if (remaining > 0) {
        snprintf(line2, sizeof(line2), "Rearmando %d s", remaining);
      } else {
        strcpy(line2, "Espere PIR...");
      }
      break;
    }
  }

  lcdPrintLine(0, line1);
  lcdPrintLine(1, line2);
  lcdDirty = false;
}
