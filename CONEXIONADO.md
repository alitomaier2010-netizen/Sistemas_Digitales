# Conexionado de la alarma (Arduino Uno)

Guía de cableado paso a paso para el proyecto **“Alarma con sensor de movimiento y clave de desactivación”**.

Este documento cubre la **versión simplificada para Arduino Uno** (sketch `Alarma_PIR_Teclado_Uno/Alarma_PIR_Teclado_Uno.ino`), que no usa LCD. Al final se indica cómo agregar el LCD 16×2 con módulo I2C si se quiere la versión completa del manual.

> **Antes de empezar:** desconectar el USB y la fuente externa. No mover cables con el Arduino energizado.

## 1. Materiales

| # | Componente | Cantidad | Uso |
|---|---|---:|---|
| 1 | Arduino Uno R3 | 1 | Controlador |
| 2 | Sensor PIR HC-SR501 | 1 | Detección de movimiento |
| 3 | Teclado matricial 4×4 | 1 | Ingreso de la clave |
| 4 | LED verde 5 mm | 1 | Indicador de reposo |
| 5 | LED rojo 5 mm | 1 | Indicador de alarma |
| 6 | Buzzer pasivo (piezoeléctrico) | 1 | Alarma sonora |
| 7 | Resistencia 330 Ω | 2 | Limitación de los LEDs |
| 8 | Llave interruptor de 2 puntos | 1 | Encendido general |
| 9 | Protoboard de 830 puntos | 1 | Montaje |
| 10 | Cables macho/macho, macho/hembra y hembra/hembra | 1 set | Conexiones |
| 11 | Fuente de 7–12 V / 1 A o cable USB | 1 | Alimentación |
| 12 | LCD 16×2 con módulo I2C | 1 | Opcional (versión completa) |

El potenciómetro de 10 kΩ y la resistencia de 220 Ω **solo** hacen falta si se usa un LCD paralelo sin módulo I2C; con el módulo I2C no se necesitan.

## 2. Mapa de pines

| Componente | Terminal | Pin del Arduino | Modo |
|---|---|---|---|
| PIR HC-SR501 | VCC | **5V** | Alimentación |
| PIR HC-SR501 | GND | **GND** | Alimentación |
| PIR HC-SR501 | OUT | **D2** | `INPUT` |
| LED verde | Ánodo (pata larga) | **D3** | `OUTPUT` |
| LED verde | Cátodo (pata corta) | **GND** | — |
| LED rojo | Ánodo (pata larga) | **D4** | `OUTPUT` |
| LED rojo | Cátodo (pata corta) | **GND** | — |
| Buzzer pasivo | Positivo (`+`) | **D5** | `OUTPUT` (`tone()`) |
| Buzzer pasivo | Negativo | **GND** | — |
| Teclado 4×4 | Fila 1 (R1) | **D6** | Escaneo |
| Teclado 4×4 | Fila 2 (R2) | **D7** | Escaneo |
| Teclado 4×4 | Fila 3 (R3) | **D8** | Escaneo |
| Teclado 4×4 | Fila 4 (R4) | **D9** | Escaneo |
| Teclado 4×4 | Columna 1 (C1) | **A0** | Escaneo |
| Teclado 4×4 | Columna 2 (C2) | **A1** | Escaneo |
| Teclado 4×4 | Columna 3 (C3) | **A2** | Escaneo |
| Teclado 4×4 | Columna 4 (C4) | **A3** | Escaneo |
| LCD 16×2 I2C (opcional) | SDA | **A4** | I2C |
| LCD 16×2 I2C (opcional) | SCL | **A5** | I2C |
| Llave de 2 puntos | — | — | En serie con la alimentación externa |

**D0 y D1 quedan libres** (son RX/TX del puerto USB). No conectar nada ahí si se va a usar el monitor serie.

## 3. Diagrama

```text
                          ARDUINO UNO R3
                       ┌─────────────────┐
     PIR OUT ──────────┤ D2              │
     LED verde ────────┤ D3              │
     LED rojo ─────────┤ D4              │
     Buzzer (+) ───────┤ D5              │
     Teclado R1 ───────┤ D6              │
     Teclado R2 ───────┤ D7              │
     Teclado R3 ───────┤ D8              │
     Teclado R4 ───────┤ D9              │
                       │                 │
     Teclado C1 ───────┤ A0              │
     Teclado C2 ───────┤ A1              │
     Teclado C3 ───────┤ A2              │
     Teclado C4 ───────┤ A3              │
                       │                 │
     5V ───────────────┤ 5V         GND ─┼──── GND común
                       └─────────────────┘
```

**Rieles del protoboard.** Llevar un cable del **5V** del Arduino al riel rojo (+) y un cable del **GND** al riel azul o negro (−). Todo el circuito se alimenta desde esos dos rieles. Si el protoboard no une los rieles de ambos lados, puentearlos con un cable.

## 4. Montaje paso a paso

1. **Rieles.** 5V del Arduino → riel rojo. GND del Arduino → riel azul. Verificar que ambos rieles estén unidos en toda la placa.
2. **PIR HC-SR501.** `VCC` → 5V, `GND` → GND, `OUT` → D2. Dejar el jumper en **H** (disparo repetido) y el potenciómetro `Tx` al mínimo para que la detección sea corta.
3. **LED verde.** Ánodo → resistencia de 330 Ω → D3. Cátodo → GND.
4. **LED rojo.** Ánodo → resistencia de 330 Ω → D4. Cátodo → GND.
5. **Buzzer pasivo.** Terminal `+` → D5. Terminal `−` → GND.
6. **Teclado 4×4.** Los cuatro primeros contactos del conector (R1–R4) → D6, D7, D8, D9. Los cuatro últimos (C1–C4) → A0, A1, A2, A3. Verificar el orden con la sección 5.
7. **Llave de 2 puntos.** En serie con el positivo de la fuente externa, entre la fuente y el conector de barril. La llave **no corta el USB**: si el Arduino está conectado a la PC, hay que desconectar el cable para apagarlo por completo.
8. **Verificar** con el multímetro en continuidad entre 5V y GND: no debe haber cortocircuito.
9. **Energizar** por USB o con la fuente y la llave cerrada.

## 5. Verificar el orden del teclado

El conector de membrana suele tener ocho contactos en este orden: **R1, R2, R3, R4, C1, C2, C3, C4**, pero el orden cambia entre fabricantes. Es la causa más común de fallas.

### 5.1. Con el multímetro

1. Poner el multímetro en continuidad.
2. Presionar una tecla de la primera fila y buscar qué dos contactos se cierran: uno es la fila, el otro la columna.
3. Repetir con una tecla de cada fila y de cada columna.
4. Anotar el orden real y conectarlo a los pines correspondientes.

### 5.2. Con un sketch de prueba

Cargar este sketch **antes** del código de la alarma y abrir el monitor serie a 9600 baudios:

```cpp
#include <Keypad.h>

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {6, 7, 8, 9};
byte colPins[COLS] = {A0, A1, A2, A3};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(9600);
  Serial.println(F("Presiona teclas..."));
}

void loop() {
  char k = keypad.getKey();
  if (k) {
    Serial.print(F("Tecla: "));
    Serial.println(k);
  }
}
```

| Síntoma | Causa probable | Solución |
|---|---|---|
| Aparece la tecla correcta | Todo bien | Continuar |
| Presionar `1` y sale `4` | Filas invertidas | Dar vuelta el orden de los cuatro cables de fila |
| Presionar `1` y sale `2` | Columnas corridas | Revisar el orden de las columnas |
| Presionar `1` y sale `A` | Filas y columnas intercambiadas | Invertir los dos grupos de cables |
| No aparece nada | Contacto flojo o cable mal puesto | Revisar continuidad y conexiones |

## 6. Alimentación

| Opción | Conexión | Observaciones |
|---|---|---|
| USB de la PC | Cable USB al Arduino | Hasta 500 mA; la llave no corta esta alimentación |
| Fuente 7–12 V / 1 A | Conector de barril, con la llave en serie | Recomendada para la exposición |
| Batería de 9 V | Conector de barril, con la llave en serie | Autonomía de pocas horas |

Consumo estimado sin LCD: unos **160 mA** (Arduino 50 mA + PIR 65 mA + LEDs 18 mA + buzzer 30 mA). Con LCD I2C se agregan unos 20 mA.

**No alimentar el conector de barril con 5 V**: el regulador de la placa necesita al menos 7 V para entregar 5 V estables.

## 7. Agregar el LCD (versión completa)

Si se usa la versión completa del manual (sección 6.1), agregar el LCD 16×2 con módulo I2C:

| Pin del módulo | Pin del Arduino |
|---|---|
| VCC | 5V |
| GND | GND |
| SDA | A4 |
| SCL | A5 |

Ajustar el contraste con el potenciómetro del módulo. La dirección I2C habitual es `0x27`; si no responde, probar `0x3F`.

## 8. Lista de verificación antes de energizar

- [ ] Multímetro en continuidad entre 5V y GND: no debe pitar.
- [ ] Cada LED tiene su resistencia de 330 Ω y el cátodo va a GND.
- [ ] El PIR está alimentado con **5 V**, no con 3,3 V.
- [ ] Nada conectado en D0 ni D1.
- [ ] Ningún cable suelto tocando dos filas del protoboard.
- [ ] El orden del teclado fue verificado (sección 5).
- [ ] La llave está abierta antes de conectar la fuente.

## 9. Problemas comunes

| Síntoma | Causa probable | Solución |
|---|---|---|
| Un LED no enciende | Polaridad invertida o falta la resistencia | Pata larga al pin, corta a GND; revisar la resistencia |
| El buzzer hace solo un clic | Es un buzzer activo, no pasivo | Usar `digitalWrite()` en lugar de `tone()` |
| El PIR nunca detecta | Alimentación incorrecta, `OUT` mal conectado o sensibilidad baja | Medir 5 V, revisar D2, ajustar `Sx` |
| El PIR siempre en HIGH | Retardo `Tx` alto o fuente de calor cercana | Bajar `Tx`, alejar de ventanas y del buzzer |
| Falsos disparos al encender | Calentamiento del PIR | Esperar los 60 s; el LED verde parpadea mientras tanto |
| El teclado marca teclas equivocadas | Filas y columnas invertidas | Corregir el cableado con la sección 5 |
| El Arduino se reinicia al sonar | Consumo alto o fuente débil | Usar fuente de 1 A o un transistor para el buzzer |
| El monitor serie muestra basura | Velocidad distinta de 9600 | Configurar 9600 baudios |

## 10. Cargar el sketch

1. Instalar el Arduino IDE 2.x.
2. Instalar la librería **Keypad** desde el Gestor de librerías.
3. Abrir `Alarma_PIR_Teclado_Uno/Alarma_PIR_Teclado_Uno.ino`.
4. Seleccionar **Herramientas → Placa → Arduino AVR Boards → Arduino Uno**.
5. Seleccionar el puerto en **Herramientas → Puerto**.
6. Presionar **Subir**.
7. Abrir el monitor serie a **9600 baudios** para ver las trazas.

Para probar sin esperar el calentamiento, cambiar `WARMUP_MS` a `10000UL` y volver a `60000UL` antes de la entrega.
