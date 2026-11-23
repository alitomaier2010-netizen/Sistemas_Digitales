# Sistemas_Digitales

**Alarma con sensor de movimiento y clave de desactivación**  
Proyecto Integrador – Sistemas Digitales I  
EEST N°1 – 5°3

**Integrantes:** Alexis Maier, Aramis Demaria, Santino Laudati e Ian Quiroga  
**Profesor:** Lic. Prof. Nicolás Salimbeni

---

## Sobre este proyecto

Es una alarma que detecta movimiento y avisa con luz y sonido. Solo se puede manejar con una clave escrita en un teclado. Un sensor de movimiento HC-SR501 vigila la zona. Cuando la alarma está activada y detecta a alguien, se encienden un LED rojo y un zumbador. Con un teclado matricial 4×4 se activa, se desactiva y se cambia la clave.

El equipo usa una placa Arduino Uno alimentada por USB y va dentro de una carcasa impresa en 3D. Desde afuera solo se ven los dos LEDs, el sensor, el teclado, la palanca de encendido y el cable USB.

---

## Índice

1. Contenido del repositorio
2. Materiales
3. Cómo se usa
4. Cargar el programa
5. Carcasa 3D
6. Glosario
7. Preguntas frecuentes

---

## 1. Contenido del repositorio

| Ruta | Descripción |
|---|---|
| `Alarma_PIR_Arduino_Uno/` | Programa de la alarma (proyecto PlatformIO). `src/main.cpp` es el programa principal; `prueba_hardware.cpp` y `prueba_sensor.cpp` sirven para probar el hardware por partes. |
| `Armado.txt` | Cómo conectar cada componente al Arduino Uno. |
| `Manuales/MANUAL_DEL_USUARIO.md` y `.pdf` | Manual para quien usa la alarma. |
| `Manuales/imagenes/` | Ilustraciones del manual. |
| `Sistemas Digitales Parte 2.stl` | Carcasa para imprimir en 3D. |
| `Teoria/` | Consigna del proyecto y lista de componentes. |

## 2. Materiales

| Componente | Cantidad |
|---|---:|
| Placa Arduino Uno | 1 |
| Sensor de movimiento PIR HC-SR501 | 1 |
| Teclado matricial 4×4 | 1 |
| LED verde de 5 mm + resistencia de 1 kΩ | 1 |
| LED rojo de 5 mm + resistencia de 330 Ω (en serie con el zumbador) | 1 |
| Zumbador pasivo (piezoeléctrico, 5 V) | 1 |
| Palanca de encendido | 1 |
| Protoboard | 1 |
| Cables macho-macho, macho-hembra y hembra-hembra | varios |
| Cable USB tipo B (de datos) | 1 |
| Carcasa impresa en 3D | 1 |

## 3. Cómo se usa

| Acción | Teclas |
|---|---|
| Activar la alarma | `A` → clave → `#` (60 s para alejarse) |
| Apagar el sonido y seguir vigilando | `A` → clave → `#` |
| Desactivar la alarma | `B` → clave → `#` |
| Cambiar la clave | `C` → clave actual → `#` → clave nueva (4 a 8 dígitos) → `#` |
| Borrar el último dígito | `*` |

- **Clave de fábrica:** `1234`. Si se cambia con `C`, vuelve a `1234` cada vez que la alarma se apaga.
- **LED verde:** encendido cuando no hay movimiento frente al sensor.
- **LED rojo y zumbador:** se encienden cuando la alarma activada detecta a alguien.

El detalle está en el [Manual del Usuario](Manuales/MANUAL_DEL_USUARIO.md).

## 4. Cargar el programa

1. Abrir `Alarma_PIR_Arduino_Uno/` en Visual Studio Code con la extensión PlatformIO.
2. Cerrar cualquier monitor serie abierto.
3. Ejecutar `pio run -e uno -t upload`. La librería Keypad se instala sola.
4. Para ver los mensajes, abrir el monitor serie a 115200 baudios.

Los programas de prueba se cargan con `pio run -e prueba_hw -t upload` (LEDs, zumbador, teclado y sensor) y `pio run -e prueba_pir -t upload` (solo el sensor). Después hay que volver a cargar `uno`.

## 5. Carcasa 3D

El archivo `Sistemas Digitales Parte 2.stl` mide **17 × 14 × 9 cm**. Está dibujado en centímetros. Si el programa de impresión lo muestra de 17 × 14 × 9 mm, hay que escalarlo al **1000 %**.

---

## 6. Glosario

| Término | Significado |
|---|---|
| **PIR** | Sensor infrarrojo pasivo: detecta el calor de un cuerpo en movimiento. |
| **Arduino Uno** | Placa con microcontrolador que ejecuta el programa de la alarma. |
| **Teclado matricial** | Teclado de 16 teclas organizadas en 4 filas y 4 columnas. |
| **Zumbador pasivo** | Sirena pequeña que suena según la señal que le manda la placa. |
| **PlatformIO** | Extensión de Visual Studio Code para compilar y cargar programas en la placa. |
| **Monitor serie** | Ventana de la computadora que muestra los mensajes que envía la placa por USB. |
| **STL** | Formato de archivo 3D que se usa para imprimir piezas. |

## 7. Preguntas frecuentes

**¿Necesita internet o WiFi?**  
No. Funciona de forma local.

**¿Qué pasa si me olvido la clave?**  
Al apagar y volver a encender la alarma, la clave vuelve a ser `1234`.

**¿Por qué el sensor tarda en funcionar al encenderlo?**  
El HC-SR501 necesita alrededor de 1 minuto para estabilizarse. Por eso, al activar la alarma el programa espera 60 s antes de empezar a vigilar.

**No puedo cargar el programa: dice "Acceso denegado".**  
Hay un monitor serie abierto que ocupa el puerto. Cerrarlo y volver a intentar.

