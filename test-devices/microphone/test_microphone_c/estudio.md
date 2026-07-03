# Guía de estudio — Grabar audio en C (bare-metal) con el MAX9814

Tu objetivo es escribir **`record_max9814.c`**: un programa en C que haga
*exactamente el mismo trabajo* que `record_max9814.ino`, pero sin la librería de
Arduino, usando los drivers de este proyecto (`drivers/uart.*`, `drivers/adc.*`,
`drivers/registers.h`) y acceso directo a los registros del ATmega328P.

El programa final debe:

1. Muestrear el micrófono por el ADC (canal A0) a **8000 Hz**.
2. Reducir cada muestra de **10 bits (0–1023)** a **8 bits (0–255)**.
3. Enviar cada muestra como **un byte crudo (RAW)** por UART a **115200 baud**.
4. Ser compatible con `record_audio.py`, que arma el archivo `.wav`.

> **Idea clave:** el guion de Python (`record_audio.py`) **no cambia**. El
> "contrato" es el flujo de bytes: 8000 bytes/segundo, cada byte = una muestra de
> 8 bits, a 115200 baud. Si tu `.c` respeta ese contrato, Python ni se entera de
> que cambiaste el firmware.

---

## 1. Qué hace el `.ino` (línea por línea → su equivalente en C)

Antes de escribir nada, entendé el original. Esta tabla es tu hoja de ruta:

| En el `.ino` (API Arduino)        | Qué hace                                  | Equivalente bare-metal en tu `.c`                     |
|-----------------------------------|-------------------------------------------|-------------------------------------------------------|
| `Serial.begin(115200)`            | Configura la UART a 115200 baud           | `uart_init()` **compilado con `-DBAUD=115200`**       |
| `ADCSRA &= ...; ADCSRA \|= ...`   | Acelera el ADC (prescaler 128 → 16)       | Tocar `ADCSRA` con `registers.h` (ver §5.9)           |
| `analogRead(A0)`                  | Lee 10 bits del canal 0                    | `adc_read(0)`                                         |
| `micros()` + comparación          | Temporiza 125 µs sin bloquear             | Timer o `_delay_us` (ver §5.8, es lo más difícil)     |
| `lectura >> 2`                    | Pasa de 10 bits a 8 bits                   | Igual: `muestra = lectura >> 2;`                      |
| `Serial.write(muestra)`           | Envía **1 byte crudo**                     | `uart_transmit(muestra)` (¡**no** `uart_print_int`!)  |

Si entendés cada fila de esta tabla, ya tenés el 80% del programa en la cabeza.

---

## 2. Mapa de conceptos a aprender

Marcá cada uno cuando sientas que lo entendés. Los detalles están en la §5.

- [ ] **Bare-metal vs API de Arduino** — por qué acá programamos registros a mano.
- [ ] **Registros y `volatile`** — cómo se le habla al hardware desde C.
- [ ] **ADC** (conversión analógico→digital): resolución, referencia, velocidad.
- [ ] **UART / USART**: qué es, qué es el *baud rate*, por qué debe coincidir en ambos lados.
- [ ] **RAW vs ASCII** — el error #1 en este tipo de tests. Byte crudo ≠ texto.
- [ ] **Muestreo (sampling)**: *sample rate*, teorema de Nyquist, por qué 8 kHz.
- [ ] **PCM y formato WAV**: qué es "8-bit unsigned mono".
- [ ] **Temporización**: `_delay_us` vs *timers*, y el "presupuesto de tiempo".
- [ ] **Velocidad del ADC (prescaler)** — por qué el driver por defecto es *demasiado lento* para 8 kHz.

---

## 3. Herramientas que ya tenés en el proyecto

Antes de reinventar nada, mirá qué te dan los drivers (leelos, son cortos):

**`drivers/uart.h`**
```c
void    uart_init(void);            // configura la UART (usa BAUD y F_CPU)
void    uart_transmit(uint8_t d);   // envía UN byte crudo  <-- este te sirve
void    uart_print(const char* s);  // envía texto (cadena)
void    uart_print_int(uint16_t n); // envía un número como TEXTO ("512")
```

**`drivers/adc.h`**
```c
void     adc_init(void);            // enciende el ADC (ojo: prescaler 128, ver §5.9)
uint16_t adc_read(uint8_t channel); // lee 10 bits (0–1023) del canal dado
```

**`drivers/registers.h`** — te da los nombres de registros/bits del ATmega328P
(`ADCSRA`, `ADPS0..2`, etc.) para tocarlos directamente cuando el driver no
alcanza.

> Pregunta para vos: de las 4 funciones de UART, ¿cuál manda el byte tal cual,
> sin convertirlo a texto? Esa es la que necesitás. Las otras romperían el WAV.

---

## 4. Pasos de implementación (checklist ordenada)

Hacelo incremental. No escribas todo de una; validá cada paso.

1. **Arrancá de cero** el archivo `record_max9814.c` (el que hay tiene un error de
   sintaxis a propósito). Poné los `#include` de los drivers y un `main` vacío con
   `while (1) {}`.
2. **Inicializá** UART y ADC (`uart_init()`, `adc_init()`).
3. **Prueba de vida (sanity check):** en el `while`, leé el ADC y mandalo como
   *texto* con `uart_print_int` + un `"\r\n"`, con un `_delay_ms(100)`. Abrí el
   monitor serie y confirmá que ves números que cambian al hablarle al micro.
   *(Esto es solo para depurar; después lo sacás.)*
4. **Cambiá a RAW:** reemplazá el texto por `uart_transmit(lectura >> 2)`. Ya no
   verás texto legible en el monitor — es esperable, ahora son bytes binarios.
5. **Acelerá el ADC** (§5.9): si no lo hacés, no llegás a 8 kHz reales.
6. **Temporizá a 8 kHz** (§5.8): que cada muestra salga cada ~125 µs.
7. **Ajustá el `Makefile`** (§6): `TARGET`, `TEST_SRC` y `BAUD=115200`.
8. **Grabá** con `record_audio.py` y **escuchá** el `.wav`.

---

## 5. Los conceptos, explicados

### 5.1 Bare-metal vs API de Arduino
El `.ino` corre sobre la librería de Arduino: `analogRead`, `Serial`, etc., son
funciones que *por dentro* configuran registros del chip. En este proyecto hacemos
ese "por dentro" a mano. Ventaja: entendés y controlás todo (velocidad, timing).
Costo: vos configurás cada periférico. Los drivers del proyecto ya encapsulan lo
básico, así que no partís de cero.

### 5.2 Registros y `volatile`
Un microcontrolador se controla escribiendo/leyendo **registros**: direcciones de
memoria especiales conectadas al hardware. En `registers.h` verás cosas como:
```c
#define ADCSRA REG8(0x7A)   // un registro de 8 bits en la dirección 0x7A
#define ADSC   6            // el bit 6 dentro de ese registro
```
Para **poner un bit a 1**: `ADCSRA |= (1 << ADSC);`
Para **poner un bit a 0**: `ADCSRA &= ~(1 << ADSC);`
Para **preguntar si un bit está en 1**: `if (ADCSRA & (1 << ADSC)) { ... }`

Los registros se declaran `volatile` (mirá `REG8` en `registers.h`): le dice al
compilador "este valor puede cambiar solo, no optimices las lecturas". Sin
`volatile`, un `while (ADCSRA & (1<<ADSC));` podría quedarse colgado para siempre.

### 5.3 El ADC (Conversor Analógico → Digital)
El micrófono entrega un **voltaje analógico** (0–~2.45 V, con reposo en ~1.25 V).
El ADC lo convierte en un **número**. En el ATmega328P:
- Resolución **10 bits** → valores de **0 a 1023**.
- Referencia: usamos AVCC (5 V) → 0 V ≈ 0, 5 V ≈ 1023. (`adc_read` ya la fija con
  `REFS0`.)
- En silencio el micro reposa a ~1.25 V → deberías leer **~512** (¡no 0!). Esa es
  la señal de que está bien conectado.

`adc_read(0)` te devuelve esos 10 bits. Como el WAV que queremos es de 8 bits,
tenemos que "achicar" 0–1023 a 0–255. ¿Cómo? Descartando los 2 bits menos
significativos: `>> 2` (dividir por 4). Pensá por qué `1023 >> 2 == 255`.

### 5.4 UART / USART y baud rate
La **UART** es el "cable serie" por el que el Arduino le habla a la PC (por USB).
Manda bits uno atrás de otro. El **baud rate** es la velocidad (bits por segundo).
**Regla de oro:** el emisor (firmware) y el receptor (`record_audio.py` o el
monitor) **tienen que usar el mismo baud** o se lee basura.

Cuentas rápidas para este test:
- Queremos **8000 muestras/segundo**, 1 byte cada una → 8000 bytes/s.
- Cada byte por UART ocupa ~10 bits (1 start + 8 datos + 1 stop).
- A 9600 baud caben ~960 bytes/s → **no alcanza**. Por eso el `.ino` usa 115200
  (~11520 bytes/s), que sí da abasto. **Tu `.c` debe compilarse con `BAUD=115200`.**

El driver `uart.h` toma `BAUD` de un `#define`; el `Makefile` lo pasa con
`-DBAUD=...`. O sea: el cambio de baud es en el **Makefile**, no en el código.

### 5.5 RAW vs ASCII — el error más común
`uart_print_int(512)` **no** manda el número 512: manda **tres bytes de texto**,
los caracteres `'5'`, `'1'`, `'2'` (que en realidad son 53, 49, 50). Perfecto para
leer con los ojos en el monitor, **veneno** para el WAV.

Para audio necesitás el **byte crudo**: si la muestra vale 200, tiene que salir el
byte 200, ni más ni menos. Esa es exactamente `uart_transmit(muestra)`. Grabátelo:

| Función            | Qué sale por el cable para el valor 200 | ¿Sirve para el WAV? |
|--------------------|-----------------------------------------|---------------------|
| `uart_print_int`   | `'2'`, `'0'`, `'0'` (3 bytes de texto)  | ❌ No                |
| `uart_transmit`    | un solo byte de valor 200               | ✅ Sí               |

### 5.6 Muestreo (sampling): sample rate y Nyquist
Digitalizar audio = medir el voltaje muchas veces por segundo. Cuántas veces =
**sample rate** (Hz). El **teorema de Nyquist** dice que podés capturar frecuencias
de hasta *la mitad* del sample rate. Con **8000 Hz** capturás hasta ~4000 Hz:
suficiente para voz humana (la telefonía clásica usa justo 8 kHz). Por eso este
test usa 8000 Hz y no más: menos datos, y para probar el micro alcanza.

8000 muestras por segundo → una muestra cada **1/8000 s = 125 µs**. Ese 125 es el
número mágico que vas a ver por todos lados.

### 5.7 PCM y formato WAV
**PCM** (Pulse-Code Modulation) = guardar audio como una lista cruda de muestras.
El `.wav` que produce Python es **PCM 8-bit unsigned, mono, 8000 Hz**:
- *unsigned 8-bit*: cada muestra es un byte 0–255, con el silencio en ~128.
- *mono*: un solo canal.
- *8000 Hz*: 8000 muestras por segundo.

Fijate que estos tres parámetros están fijados en `record_audio.py`
(`setsampwidth(1)`, `setnchannels(1)`, `setframerate(8000)`). Tu firmware tiene que
producir exactamente ese formato de bytes; el "encabezado" del WAV lo pone Python.

#### 5.7.1 ¿Por qué 8 bits y no los 10 que entrega el ADC?
Pregunta natural: si el ADC te da **10 bits (0–1023)**, ¿por qué tirás 2 con `>> 2`
en vez de aprovecharlos? Cuatro razones, de la que **te obliga** a la que **te
conviene**:

1. **El WAV no tiene "10 bits".** El formato PCM define el ancho de muestra en
   **bytes enteros**: 8, 16, 24 o 32 bits. No existe WAV de 10 bits. Mirá
   `record_audio.py`: `setsampwidth(1)` toma **bytes** (1, 2, 3, 4). Para conservar
   los 10 bits tu única opción es meterlos en un WAV de **16 bits**
   (`setsampwidth(2)`), con 6 bits de relleno. O sea, la disyuntiva real no es
   "8 vs 10", es **"8 vs 16"** — y ahí pesan las otras tres razones.

2. **16 bits no entra en el UART.** A 8 kHz, 2 bytes por muestra = 16000 bytes/s.
   Por UART, cada byte ocupa ~10 bits (8N1) → **160000 bps**, y tu enlace es de
   115200 baud (~11520 bytes/s). **No alcanza.** Tendrías que subir el baud a 230400
   o bajar el sample rate a la mitad. Con 8 bits (8000 bytes/s) el canal queda
   holgado (~70 %).

   | Formato | Bytes/muestra | Bytes/s a 8 kHz | Bits/s en UART (×10) | ¿Entra en 115200? |
   |---------|---------------|-----------------|----------------------|-------------------|
   | 8-bit   | 1             | 8 000           | 80 000               | ✅ sí             |
   | 16-bit  | 2             | 16 000          | **160 000**          | ❌ no             |

3. **Los 2 bits que tirás son los menos confiables.** Para llegar a 8 kHz acelerás
   el ADC a prescaler 16 → reloj de **1 MHz** (§5.9). Pero el datasheet garantiza
   los 10 bits de precisión plena solo hasta **~200 kHz** de reloj de ADC. A 1 MHz,
   los bits **menos** significativos ya están contaminados por ruido de conversión —
   justo los que descarta el `>> 2`. Guardarlos sería guardar mayormente ruido.

4. **1 byte = 1 muestra es un protocolo a prueba de balas.** Con 8 bits, cada byte
   que llega **es** una muestra. Con 16 bits tenés que fijar endianness (¿byte alto
   o bajo primero?) y **mantener la sincronía**: si se pierde o se cuela un solo
   byte, todos los pares se corren y el audio entero se vuelve ruido. Con 1
   byte/muestra, un byte perdido es apenas un "clic".

> **En una frase:** el WAV no soporta 10 bits (saltaría a 16), y 16 bits no entra
> en 115200 baud a 8 kHz, duplica los datos, exige formato *signed* con endianness,
> y los 2 bits extra son justo los que el ADC acelerado degrada. Para **probar** el
> micro, 8-bit unsigned mono a 8 kHz —el estándar histórico de la telefonía— es la
> relación calidad/simplicidad correcta.

### 5.8 Temporización: `_delay_us` vs timers (la parte difícil)
Necesitás una muestra **cada 125 µs**. Hay dos caminos:

**A) Camino simple — `_delay_us()` (`#include <util/delay.h>`)**
Poner `_delay_us(125)` en el bucle *parece* que da 8 kHz, pero **no**: el período
real es `125 µs + tiempo de leer el ADC + tiempo de mandar el byte`. Si la
conversión tarda 100 µs, tu período real es ~225 µs → ~4400 Hz, no 8000. El audio
saldría *más grave y lento* de lo real. Sirve para arrancar y entender, pero es
inexacto.

**B) Camino correcto — como el `.ino`: temporización no bloqueante**
El `.ino` no usa delays: mide el tiempo *absoluto* con `micros()` y actúa cuando
pasaron 125 µs desde la muestra anterior, sumando `ultimoTiempo += 125` para no
acumular deriva. El equivalente bare-metal es un **Timer/Contador** del ATmega328P
(Timer0/Timer1/Timer2): configurás el timer para que "tictaquee" cada 125 µs y en
cada tic tomás una muestra. Esto te da 8 kHz reales.

> Sugerencia de progresión: hacelo primero con `_delay_us` (camino A) para tener
> *algo que grabe*, aunque suene raro. Después estudiá los timers y pasá al camino
> B. Los timers son un tema en sí mismo — buscá en el datasheet del ATmega328P la
> sección "8-bit Timer/Counter" y el modo CTC (Clear Timer on Compare).

#### 5.8.1 Camino B en detalle: un Timer en modo CTC (la temporización correcta)
Un **Timer/Counter** es un contador de hardware que sube solo, a un ritmo derivado
del reloj de CPU (16 MHz) dividido por un **prescaler** (igual idea que el del ADC,
§5.9, pero es *otro* prescaler, el del timer). Corre **en paralelo** a tu programa:
no importa cuánto tarde tu `while`, el timer cuenta por su cuenta.

**La idea del modo CTC (Clear Timer on Compare):** le decís al timer un valor
"tope" (en el registro `OCR`). El contador sube 0, 1, 2, …, y **cuando llega al
tope se reinicia solo a 0 y levanta una bandera** (`OCF`, output-compare flag). Esa
bandera se prende **exactamente** cada período que vos calculaste, sin importar qué
esté haciendo el resto del código → **sin deriva** (a diferencia del `_delay_us`,
donde el período real era `125 µs + lo que tarde leer y enviar`, §5.8).

**La cuenta del tope.** El período de un "tic" del contador es
`prescaler / F_CPU`. Querés que la bandera salte cada 125 µs (= 8000 Hz), así que:

```
OCR = (F_CPU / prescaler / frecuencia_muestreo) - 1
```

El `-1` es porque el contador arranca en 0 (contar de 0 a N son N+1 tics). Elegís el
prescaler de modo que el `OCR` dé **entero** y **quepa** en el registro:

| Prescaler | Tic       | Tics en 125 µs | OCR = tics−1 | ¿Sirve?                          |
|-----------|-----------|----------------|--------------|----------------------------------|
| 1         | 0,0625 µs | 2000           | 1999         | ✅ (necesita timer de 16 bits)   |
| **8**     | **0,5 µs**| **250**        | **249**      | ✅ entero y chico → el más cómodo |
| 64        | 4 µs      | 31,25          | —            | ❌ no da entero → derivaría       |

Con prescaler **8** te da `OCR = 249`. Entra hasta en un timer de 8 bits (≤255),
pero conviene usar **Timer1 (16 bits)** para no pelearte con desbordes y dejar el
Timer0 libre (Arduino lo usa para `millis`, aunque acá no importe).

**Qué registros tocar (Timer1, CTC, por *polling*).** El camino más simple es **no**
usar interrupciones: configurás el timer y en el bucle *preguntás* si ya saltó la
bandera. Los registros del Timer1 en el ATmega328P son:

| Registro   | Para qué                                              |
|------------|------------------------------------------------------|
| `TCCR1A`   | Modo de forma de onda (parte baja). Acá va en **0**. |
| `TCCR1B`   | Bit `WGM12` = modo CTC; bits `CS12:CS10` = prescaler.|
| `OCR1A`    | El **tope** calculado arriba (249).                  |
| `TIFR1`    | Tiene la bandera `OCF1A` que se prende en cada tope. |

Para prescaler 8, `CS12:CS11:CS10 = 010` → solo `CS11` en 1. Y `WGM12` en 1
selecciona CTC con tope en `OCR1A`.

> **Ojo, gotcha del proyecto:** `registers.h` **todavía no** define los registros del
> Timer1 (mirá: solo tiene UART, TWI, PORTC y ADC). Tenés dos opciones:
> 1. **Agregarlos a `registers.h`** con el patrón `REG8`/`REG16` ya existente, por
>    ejemplo `#define TCCR1B REG8(0x81)`, `#define OCR1A REG16(0x88)`,
>    `#define TIFR1 REG8(0x36)`, más los bits `WGM12`, `CS11`, `OCF1A`. (Buscá las
>    direcciones en la tabla "Register Summary" del datasheet.)
> 2. **Incluir `<avr/io.h>`**, que ya trae todos esos nombres definidos. Más rápido,
>    pero se aparta del estilo "todo a mano" del proyecto.

**Cómo se usa (esqueleto por polling).** Configurás una vez antes del `while`, y
adentro esperás la bandera, la limpiás y tomás la muestra:

```c
// --- setup del timer (una vez, antes del while) ---
TCCR1A = 0;                            // modo normal en la parte baja
TCCR1B = (1 << WGM12) | (1 << CS11);   // CTC + prescaler 8
OCR1A  = 249;                          // 125 us  (16e6/8/8000 - 1)

// --- dentro del while ---
while (!(TIFR1 & (1 << OCF1A)))        // esperar a que el timer llegue a 249
    ;                                  //   (bloquea muy poco: la conversión y el
TIFR1 = (1 << OCF1A);                  //   envío entran holgados en 125 us)
// limpiar la bandera se hace escribiendo un 1 en ella (sí, un 1, no un 0)

uint16_t read = adc_read(MIC_PIN);     // ~13 us con el ADC acelerado (§5.9)
uart_transmit(read >> 2);              // ~87 us a 115200 baud
```

Fijate que **la limpieza de la bandera es escribiendo un `1`** (particularidad del
hardware, no es un error de tipeo). Y que leer (~13 µs) + enviar (~87 µs) ≈ 100 µs
< 125 µs: por eso todo el trabajo *entra dentro* del período y el ritmo lo marca el
timer, no la UART. Si el trabajo se pasara de 125 µs, perderías tics y el ritmo se
desarmaría — por eso primero hay que acelerar el ADC (§5.9).

**Variante avanzada — interrupciones (ISR).** En vez de *preguntar* por la bandera,
podés hacer que el timer *te avise*: habilitás la interrupción de compare-match
(`TIMSK1`), escribís una función `ISR(TIMER1_COMPA_vect)` que toma la muestra, y
llamás a `sei()`. Es lo más elegante (la CPU podría dormir entre muestras), pero
suma conceptos (vector de interrupción, `<avr/interrupt.h>`, reentrancia). Para
*probar* el micrófono, el polling de arriba alcanza y sobra; dejá la ISR para cuando
el polling ya te funcione.

### 5.9 Velocidad del ADC (prescaler) — el gotcha escondido
El ADC tiene su propio reloj, derivado del reloj de CPU (16 MHz) dividido por un
**prescaler**. Una conversión tarda ~13 ciclos de *ese* reloj.

El driver `adc_init()` deja el prescaler en **128**:
`16 MHz / 128 = 125 kHz` → una conversión tarda `13 / 125000 ≈ 104 µs`.

¡Pero tu presupuesto total por muestra es de solo **125 µs**! Si la conversión ya
se come 104 µs, no te queda casi nada para el resto y no llegás a 8 kHz. Por eso el
`.ino` **acelera el ADC** bajando el prescaler a **16**:
`16 MHz / 16 = 1 MHz` → conversión `13 / 1e6 ≈ 13 µs`. Mucho margen.

Tu tarea: después de `adc_init()`, ajustar los bits `ADPS2:ADPS1:ADPS0` de
`ADCSRA` para prescaler 16. En la tabla del datasheet, prescaler 16 = `100`. O sea:
poné `ADPS2` en 1 y `ADPS1`, `ADPS0` en 0. Mirá cómo lo hace el `.ino` (líneas
22–23) y traducilo usando `registers.h`. (Nota: a 1 MHz el ADC pierde un poco de
precisión en los bits bajos, pero como igual tiramos 2 bits con `>> 2`, no
importa.)

#### 5.9.1 De dónde sale el 128 y cómo llegar al 16 (paso a paso)
El ADC no corre a los 16 MHz de la CPU: tiene su **propio reloj**, que sale de
dividir los 16 MHz por el **prescaler**. Cada conversión tarda ~13 ciclos de *ese*
reloj. Tu presupuesto por muestra es fijo (125 µs para 8 kHz), así que todo el
juego es que **una conversión entre cómoda en esos 125 µs**:

| Prescaler | Reloj del ADC     | Conversión (13 ciclos) | ¿Cabe en 125 µs?              |
|-----------|-------------------|------------------------|-------------------------------|
| **128** (default) | 16 MHz / 128 = 125 kHz | 13 / 125 000 ≈ **104 µs** | Apenas — no queda tiempo para nada más |
| **16** (acelerado) | 16 MHz / 16 = 1 MHz    | 13 / 1e6 ≈ **13 µs**      | Sí, con muchísimo margen      |

**¿Dónde está ese 128 por defecto?** En `drivers/adc.c`, dentro de `adc_init()`:
```c
ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
```
Los tres bits `ADPS2 ADPS1 ADPS0` (ADC **P**re**S**caler) fijan el divisor. Ahí
están **los tres en 1** → binario `111` → prescaler 128. Por eso hay que
corregirlos *después* de llamar a `adc_init()`.

**La tabla del datasheet** (los 3 bits forman un número que elige el divisor):

| ADPS2 | ADPS1 | ADPS0 | Prescaler        |
|:-----:|:-----:|:-----:|:----------------:|
| 1 | 1 | 1 | 128 ← el default |
| **1** | **0** | **0** | **16 ← el que querés** |

O sea: **dejar `ADPS2` en 1, y poner `ADPS1` y `ADPS0` en 0.**

**Cómo lo hace el `.ino`** (líneas 22–23), leído bit por bit:
```c
ADCSRA &= ~(bit(ADPS0) | bit(ADPS1)); // apaga (0) ADPS0 y ADPS1
ADCSRA |= bit(ADPS2);                 // enciende (1) ADPS2  → queda 1 0 0 = /16
```
- `&= ~(...)` es el patrón de **poner bits a 0** de la §5.2: la máscara junta los
  dos bits, `~` la invierte y `&=` apaga *solo* esos, sin tocar el resto.
- `|= ...` es el patrón de **poner un bit a 1**: asegura `ADPS2` encendido.

**Traducción a tu `.c`:** idéntico, cambiando el `bit(x)` de Arduino por el
`(1 << x)` de bare-metal. Tu `registers.h` ya define `ADCSRA` y `ADPS0..2`, así
que la traducción es directa (esto va en el TODO §5.9 del esqueleto de la §10).

---

## 6. Compilar, cargar y probar

### 6.1 Ajustar el `Makefile`
El `Makefile` actual apunta al otro test. Para el tuyo necesitás cambiar el nombre
del objetivo, el fuente y el baud. Buscá estas líneas y adaptalas:
```make
BAUD     = 115200          # <-- clave para 8 kHz (§5.4)
TARGET   = record_max9814
TEST_SRC = record_max9814.c
```
`BAUD` viaja al compilador vía `-DBAUD=$(BAUD)` (mirá `CFLAGS`) y de ahí a
`uart.h`. Los drivers `uart.c` y `adc.c` ya están en `DRIVER_SRC`, no toques eso.

### 6.2 Compilar y cargar
```bash
make          # compila; mirá que no haya warnings raros
make upload   # graba el .hex en el Arduino (auto-detecta el puerto)
```

### 6.3 Grabar el audio
El protocolo es idéntico al del `.ino`, así que reutilizás el mismo script
compartido (`scripts/record_audio.py`). Corrélo **desde esta carpeta** para que el
`.wav` se guarde acá:
```bash
uv run ../../../scripts/record_audio.py     # graba 5 s -> ./grabacion.wav
```
> Ojo con el monitor serie: si dejás `make monitor` abierto ocupando el puerto,
> Python no va a poder abrirlo. Cerrá uno antes de usar el otro.

### 6.4 Escuchar
```bash
aplay grabacion.wav      # Linux
# o
ffplay grabacion.wav
```

---

## 7. Errores comunes / cómo depurar

| Síntoma                                   | Causa probable                                              |
|-------------------------------------------|------------------------------------------------------------|
| El `.wav` es ruido/basura ininteligible   | Usaste `uart_print_int` en vez de `uart_transmit` (§5.5)   |
| Basura también en el monitor de texto     | Baud distinto entre firmware y PC (§5.4) — ¿pusiste 115200?|
| El audio suena grave/lento/estirado       | Sample rate real < 8 kHz: ADC lento (§5.9) o delay malo (§5.8) |
| El `while (ADCSRA...)` se cuelga           | Olvidaste `volatile` / no usaste los registros de `registers.h` |
| Todo en silencio (valor fijo ~0 o ~1023)  | Cableado del micro: OUT→A0, VDD→5V, GND→GND                 |
| Silencio pero valor ~512 estable          | ¡El micro está bien! Solo no hay sonido; hablale de cerca  |
| Python dice "no se recibieron datos"       | Firmware no envía / puerto ocupado por el monitor          |

**Truco de oro para depurar:** volvé temporalmente al modo texto
(`uart_print_int(lectura)` + `"\r\n"` + `_delay_ms(100)`) y abrí `make monitor`.
Si ves ~512 en silencio y sube al hablar, el ADC y el cableado están perfectos, y
el problema está solo en el formato de salida o el timing.

---

## 8. Preguntas de autoevaluación

Si podés responderlas, estás listo para escribir el `.c`:

1. ¿Por qué `uart_transmit` y no `uart_print_int` para las muestras de audio?
2. ¿Por qué 512 (y no 0) es el valor esperado en silencio?
3. ¿Por qué `>> 2` convierte 10 bits en 8 bits? ¿Cuánto da `1023 >> 2`?
4. ¿Por qué 9600 baud no alcanza y 115200 sí? (hacé la cuenta de bytes/segundo)
5. Si dejás el prescaler del ADC en 128, ¿a qué frecuencia real terminarías
   muestreando, aproximadamente?
6. ¿Qué frecuencia de audio máxima podés capturar a 8000 Hz de sample rate?
7. ¿Por qué `_delay_us(125)` **no** produce exactamente 8000 Hz?
8. ¿Por qué el WAV es de 8 bits y no aprovecha los 10 del ADC? Dá al menos dos
   razones (pista: pensá en el formato WAV y en el ancho de banda del UART).

---

## 9. Referencias

- **Datasheet ATmega328P** — secciones "ADC" (registros `ADMUX`, `ADCSRA`,
  prescaler) y "USART". Es la fuente de verdad.
- `record_max9814.ino` (en `../test_microphone_ino/`) — tu referencia funcional.
- `drivers/uart.c`, `drivers/adc.c`, `drivers/registers.h` — tus herramientas.
- README de este directorio y el README principal (§ Configuración del Entorno).
- [Nyquist–Shannon (Wikipedia)](https://es.wikipedia.org/wiki/Teorema_de_muestreo_de_Nyquist-Shannon)
- [Datasheet MAX9814](https://www.analog.com/media/en/technical-documentation/data-sheets/max9814.pdf)

---

## 10. Esqueleto para arrancar (con TODOs — completalo vos)

No es la solución: es el andamio. Copiá esto a `record_max9814.c` y andá
completando los `TODO`. Cuando termines, borrá los comentarios de andamiaje.

```c
/*
 * record_max9814.c
 * Grabación de audio con MAX9814 en C bare-metal (ATmega328P).
 * 8000 Hz, 8-bit PCM unsigned. Compatible con record_audio.py.
 */

#include <util/delay.h>          // para _delay_us / _delay_ms (camino simple)

#include "../../drivers/uart.h"
#include "../../drivers/adc.h"
#include "../../drivers/registers.h"   // para tocar ADCSRA (acelerar el ADC)

#define MIC_PIN 0

int main(void) {
    // 1) Inicializar periféricos
    uart_init();
    adc_init();

    // 2) TODO (§5.9): acelerar el ADC a prescaler 16.
    //    Pista: limpiar ADPS0 y ADPS1, dejar ADPS2 en 1 (100 = /16).
    //    Mirá las líneas 22-23 del .ino y traducilas con registers.h.

    while (1) {
        // 3) TODO (§5.8): esperar hasta que corresponda la próxima muestra (125 µs).
        //    Camino A (para empezar): un _delay_us(...).
        //    Camino B (correcto): un timer que marque 125 µs.

        // 4) Leer 10 bits del micrófono.
        uint16_t lectura = adc_read(MIC_PIN);

        // 5) TODO (§5.3): reducir de 10 bits a 8 bits.
        //    uint8_t muestra = ... ;

        // 6) TODO (§5.5): enviar la muestra como BYTE CRUDO (no como texto).
        //    ¿Cuál de las funciones de uart.h manda un solo byte tal cual?
    }

    return 0;
}
```

**Antes de dar por terminado:** repasá el checklist de la §4 y las preguntas de la
§8. Si algo del camino B (timers) te traba, entregá primero el camino A funcionando
y después iterá — es mejor tener algo que grabe que quedar bloqueado en el timing
perfecto.
