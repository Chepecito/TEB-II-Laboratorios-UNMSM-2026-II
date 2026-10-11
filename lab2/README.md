# TEB II – Laboratorio 2: Caracterización de un sensor de presión manométrica con la Ley de Boyle

Taller de Equipos Biomédicos II · Escuela Profesional de Ingeniería Biomédica · UNMSM

Se caracteriza estáticamente el sensor piezorresistivo **MPS20N0040D-D** (0–40 kPa), digitalizado con el conversor sigma-delta de 24 bits **HX710B** y leído por un **ESP32**. La presión de referencia no viene de un manómetro patrón: se calcula con la Ley de Boyle en un circuito neumático cerrado jeringa–llave de tres vías–manguera–sensor.

## Objetivos

- Leer con el ESP32 la salida del HX710B, verificar el protocolo con el osciloscopio y determinar el cero y el ruido del sensor a presión atmosférica.
- Medir con agua el volumen interno de la llave y la manguera (volumen muerto).
- Armar el circuito neumático cerrado y verificar su hermeticidad.
- Registrar la respuesta del sensor frente a presiones de referencia calculadas con la Ley de Boyle, en compresión y descompresión, durante tres ciclos.
- Determinar sensibilidad, linealidad, error máximo, histéresis y resolución, y contrastarlas con la hoja de datos y con los requisitos de la ventilación mecánica.

## Fundamento en breve

| Concepto | Expresión |
|---|---|
| Ley de Boyle en el circuito cerrado | P_atm · (V₀ + V_m) = (P_atm + P_ref) · (V_j + V_m) |
| Presión manométrica de referencia (ec. 1) | P_ref = P_atm · (V₀ − V_j) / (V_j + V_m) |
| Marca de parada del émbolo (ec. 2) | V_j,min = (V₀ − r · V_m) / (1 + r), con r = P_max / P_atm |
| Modelo de ajuste | Cuentas = S · P_ref + C₀ |
| Resolución | σ₀ / S |

V₀ es el volumen inicial de la jeringa (1,00 mL), V_j el volumen al que se lleva el émbolo y V_m el volumen muerto (llave, manguera, punta de la jeringa y cavidad del sensor). Con una jeringa de 1 mL, V_m es comparable a V₀ y domina el resultado, por eso se mide con agua antes del ensayo. La compresión se hace lentamente y se espera a que la lectura se estabilice para que el proceso sea isotérmico.

## Estructura

```
firmware/codigo_Laboratorio2_ESP32_HX710B/   Sketch de Arduino usado en el laboratorio
data/barrido_lab2.csv                        Barrido de presiones (Tabla 3 del informe)
data/condiciones_sesion.csv                  Condiciones de la sesión y datos de referencia (Tabla 1)
docs/esquematico_labo2.pdf                   Esquemático exportado
hardware/kicad/                              Proyecto KiCad 10 (esquemático editable)
```

## Circuito

Esquemático: [`docs/esquematico_labo2.pdf`](docs/esquematico_labo2.pdf) (fuente editable en [`hardware/kicad/`](hardware/kicad/)).

| Módulo MPS20N0040D-D + HX710B | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| DOUT (OUT) | GPIO32 |
| SCK | GPIO33 |

El ESP32 se alimenta por USB desde el PC, que también sirve de Monitor Serial. En el esquemático el conversor está dibujado con el símbolo del HX711, de la misma familia y con el mismo protocolo serie de dos líneas.

**Materiales:** ESP32, módulo MPS20N0040D-D con HX710B, 2 jeringas de 1 mL (una para agua y otra seca para el ensayo), llave de tres vías, manguera, jumpers, osciloscopio y PC.

## Firmware

[`codigo_Laboratorio2_ESP32_HX710B.ino`](firmware/codigo_Laboratorio2_ESP32_HX710B/codigo_Laboratorio2_ESP32_HX710B.ino) es el programa que se cargó en el ESP32 durante el laboratorio:

- Lee el HX710B por *bit-banging* (espera DOUT en bajo, 24 pulsos de SCK con el MSB primero y un pulso 25 de cierre) y entrega la lectura como entero con signo (complemento a dos de 24 bits).
- Promedia N = 20 lecturas consecutivas y calcula la media y la desviación estándar muestral.
- Tiene como constantes configurables P_atm (kPa), V₀ (mL) y V_m (mL), y muestra al iniciar la marca de parada V_j,min para P_max = 35 kPa.
- Recibe por el Monitor Serial el ciclo, el sentido y el volumen V_j, calcula P_ref y rechaza los puntos con V_j fuera de rango o con P_ref mayor a 40 kPa.

Monitor Serial a **115200 baudios**, terminación *Nueva línea*.

| Comando | Acción | Salida |
|---|---|---|
| `Z` | Promedio con el puerto abierto al aire (nivel de cero) | `ZERO,media,desv,N` |
| `1,C,0.94` | Punto del barrido: ciclo 1, compresión, V_j = 0,94 mL (`D` para descompresión) | `ciclo,sentido,Vj,Pref,media,desv,N` |

## Procedimiento

1. **Etapa A – Lectura a presión atmosférica.** Conectar el módulo al ESP32, observar SCK y DOUT en el osciloscopio y registrar el cero (C₀,ᵢ) y el ruido (σ₀) con el puerto abierto al aire.
2. **Etapa B – Volumen de la llave y la manguera.** Llenarlas con agua desde una jeringa y medir el volumen inyectado dos veces (V_a1, V_a2). Secarlas por completo antes de conectarlas al sensor: el sensor solo admite gases limpios y secos.
3. **Etapa C – Montaje.** Cargar P_atm, V₀ y V_m en el programa, armar el circuito cerrado, ventear y hacer la prueba de hermeticidad en la marca de parada.
4. **Etapa D – Barrido.** Comprimir en pasos de 0,06 mL (1,00 → 0,52 mL) y descomprimir por los mismos volúmenes, tres ciclos, venteando al inicio de cada uno. Al terminar, registrar el cero final (C₀,f).

## Resultados

Condiciones de la sesión (10/10/2026):

| Dato | Valor |
|---|---|
| Cero inicial C₀,ᵢ | 3 869 164 cuentas |
| Ruido σ₀ (N = 20) | 224,46 cuentas |
| Volumen de llave y manguera | V_a1 = 0,78 mL, V_a2 = 0,76 mL (media 0,77 mL) |
| P_atm usada | 101,3 kPa |
| Marca de parada V_j,min | 0,55 mL |
| Cero final C₀,f | 3 883 265,65 cuentas |

Parámetros del ajuste:

| Parámetro | Valor |
|---|---|
| Sensibilidad S | 151 671,73 cuentas/kPa |
| Offset de la recta C₀ | 5 775 975,10 cuentas |
| R² | 0,505 |
| Error máximo | 8,20 kPa (83,66 cmH₂O), 20,5 % del alcance |
| Histéresis | No determinable |
| Resolución σ₀ / S | 0,00148 kPa (0,0151 cmH₂O) |
| Deriva del cero | 14 101,65 cuentas ≈ 0,093 kPa (≈ 63 σ₀) |

El barrido completo está en [`data/barrido_lab2.csv`](data/barrido_lab2.csv). Los valores de P_ref de ese archivo son los que calculó el programa durante la sesión con V_m = 0,76 mL (valor provisional).

**Lo que se observó:**

- **El protocolo es correcto.** En el osciloscopio se contaron 25 pulsos de SCK por lectura, lo que corresponde a entrada diferencial a 10 Hz según la hoja de datos del HX710B.
- **El conversor se satura.** El offset del sensor ocupa el 46 % del rango positivo del conversor, así que la lectura llega al máximo (8 388 607 cuentas) desde V_j = 0,70 mL y el resto de la compresión no se puede medir. Con 3,3 V de referencia, el rango de entrada del HX710B es menor que la salida del puente sumada a su offset.
- **Hay una fuga en el circuito neumático.** En la descompresión las lecturas no regresan al cero: al volver a 1,00 mL la lectura es −5 413 621 cuentas, muy por debajo del cero inicial. Por eso la histéresis no se puede determinar.
- **La resolución es buena, la exactitud no.** El ruido permite distinguir cambios de 0,015 cmH₂O, pero el error máximo (83,66 cmH₂O) supera por mucho la tolerancia de pocos cmH₂O que se exige al monitorear la presión en la vía aérea.

**Conclusión.** Tal como está, el sistema no es apto para monitorear la presión en la vía aérea en todo el alcance del sensor. Hace falta evitar la saturación del conversor, mejorar la hermeticidad del circuito y reducir la incertidumbre en la lectura del volumen de la jeringa, que es la fuente dominante.

## Referencias

- Avia Semiconductor. *HX710B: 24-Bit Analog-to-Digital Converter (ADC)* – Hoja de datos.
- *MPS20N0040D-D Pressure Sensor* – Hoja de datos.
- Çengel, Y. A., & Boles, M. A. (2015). *Thermodynamics: An Engineering Approach* (8.ª ed.). McGraw-Hill Education.
- Fraden, J. (2016). *Handbook of Modern Sensors* (5.ª ed.). Springer.
- JCGM (2008). *Evaluation of measurement data – Guide to the expression of uncertainty in measurement* (JCGM 100:2008).
- Pallás Areny, R., & Webster, J. G. (2001). *Sensors and Signal Conditioning* (2.ª ed.). John Wiley & Sons.
