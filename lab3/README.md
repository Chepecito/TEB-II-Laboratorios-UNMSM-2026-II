# TEB II – Laboratorio 3: Canal de medición de oxígeno de una máquina de anestesia

Taller de Equipos Biomédicos II · Escuela Profesional de Ingeniería Biomédica · UNMSM

Se arman y verifican las dos primeras etapas del canal de medición de oxígeno (FiO₂) de una máquina de anestesia. La primera es una celda galvánica de O₂ simulada con el DAC del ESP32 y un divisor resistivo (punto de prueba TP1). La segunda es un amplificador no inversor con el TL084N (TP2), diseñado para entregar 10 mV por cada punto porcentual de O₂.

## Objetivos

- Comprobar que TP1 entrega los milivoltios esperados para 0, 21, 50 y 100 % de O₂ y, si algún valor no coincide, determinar si el error viene del ESP32 o del divisor (Etapa A).
- Medir el offset del amplificador con la entrada a tierra (Etapa B).
- Medir la ganancia real del amplificador y compararla con la de diseño, G = 21 (Etapa B).

## Fundamento en breve

| Concepto | Expresión |
|---|---|
| Celda galvánica | I_celda = k · P_O₂, V_celda = I_celda · R_L |
| Tensión del DAC (8 bits) | V1 = (n / 255) · V0 |
| Celda simulada (TP1) | TP1 = V1 · R2 / (R1 + R2) → 10 mV en aire, 47,6 mV en O₂ puro |
| Amplificador no inversor (TP2) | G = 1 + (Rf1 + Rf2) / Rg = 21 → 0,21 V en aire, 1,00 V en O₂ puro |
| Ganancia real | Pendiente de la recta TP2 = G_real · TP1 + b (mínimos cuadrados) |
| Calibración de dos puntos | FiO₂ = 21 + (V − V21) · (100 − 21) / (V100 − V21) |

La celda galvánica es simple, autoalimentada y barata, pero su sensibilidad disminuye con el uso. Por eso el monitor de la máquina de anestesia se calibra periódicamente en aire (21 %) y en oxígeno puro (100 %). Esa misma calibración corrige los errores de ganancia y de offset de todo el canal.

## Estructura

```
firmware/TBIII_LABO_3_CODIGO/        Sketch de Arduino usado en el laboratorio
data/resistencias.csv                Resistencias nominales y medidas (Tabla 1)
data/mediciones.csv                  V1, TP1 y TP2 para f0, f21, f50 y f100 (Tablas 2 y 4)
data/condiciones.csv                 V0, alimentación y offset (Tabla 3)
analysis.py                          Divisor, ganancia real, ajuste del DAC y calibración de dos puntos
docs/informe/informe_laboratorio_3.pdf   Informe final del grupo
```

## Circuito

| Etapa | Conexión |
|---|---|
| A – Celda simulada | GPIO25 → R1 (10 kΩ) → TP1 → R2 (180 Ω) → GND |
| B – Amplificador | TP1 → pin 3 (IN+) del TL084N. Rg (1 kΩ) entre el pin 2 y GND. Rf1 + Rf2 (10 kΩ + 10 kΩ) entre el pin 2 y el pin 1. Pin 1 = TP2 |
| Alimentación | Fuente dual ±12 V: +12 V al pin 4 y −12 V al pin 11, con 100 nF de desacoplo en cada pin. Los op-amps 2, 3 y 4 sin usar quedan como seguidores con la entrada a GND |

El GND del ESP32 y el común de la fuente comparten el mismo riel del protoboard.

**Materiales:** ESP32 DevKit V1, TL084N, 3 resistencias de 10 kΩ, 1 de 1 kΩ y 1 de 180 Ω, 2 capacitores de 100 nF, protoboard, fuente dual, multímetro Fluke 115 y osciloscopio Tektronix TBS1154.

## Firmware

[`TBIII_LABO_3_CODIGO.ino`](firmware/TBIII_LABO_3_CODIGO/TBIII_LABO_3_CODIGO.ino) es el programa que se cargó en el ESP32; en el informe se llama `simulador_o2.ino`. Calcula el código del DAC que hace que TP1 entregue la tensión de una celda con 10 mV en aire, teniendo en cuenta el divisor nominal R2 / (R1 + R2) ≈ 0,0177.

- **Modo fijo:** arranca en FiO₂ = 21 %. Con `f<valor>` se fija cualquier FiO₂ entre 0 y 100 %.
- **Modo reproducción:** con `r` reproduce 504 valores reales de FiO₂ (35–96 %) del caso 95 de VitalDB, una muestra cada 0,5 s (unas 14 veces más rápido que el registro original). Con `p` se pausa y se reanuda.
- Por el Monitor Serial muestra la FiO₂, la tensión teórica de la celda y el código del DAC.

| Comando | FiO₂ | Código DAC | V1 esperado | TP1 esperado | TP2 ideal |
|---|---|---|---|---|---|
| `f0` | 0 % | 0 | 0,000 V | 0,0 mV | 0,00 V |
| `f21` | 21 % | 44 | 0,569 V | 10,0 mV | 0,21 V |
| `f50` | 50 % | 104 | 1,346 V | 23,8 mV | 0,50 V |
| `f100` | 100 % | 208 | 2,692 V | 47,6 mV | 1,00 V |

Monitor Serial a **115200 baudios**, terminación *Nueva línea*. Placa **ESP32 Dev Module**.

## Procedimiento

1. **Etapa A – Celda simulada.** Medir las resistencias, cargar el programa y armar el divisor. Medir V0 en el pin 3V3 y, con `f0`, `f21`, `f50` y `f100`, medir V1 en GPIO25 y la tensión en TP1.
2. **Etapa B – Amplificador.** Alimentar el TL084N con ±12 V y armar el amplificador no inversor. Medir el offset en TP2 con la entrada a GND. Luego medir TP1 y TP2 en los cuatro niveles y observar TP2 en el osciloscopio con `f50`.

## Resultados

Datos en [`data/`](data). Resultados calculados con `analysis.py`:

| Comando | V1 medido | TP1 medido | TP2 medido | FiO₂ sin calibrar | FiO₂ con calibración de dos puntos |
|---|---|---|---|---|---|
| `f0` | 0,108 V | 2 mV | 0,015 V | 1,5 % | 0,1 % |
| `f21` | 0,637 V | 14 mV | 0,259 V | 25,9 % | 21,0 % |
| `f50` | 1,373 V | 30 mV | 0,597 V | 59,7 % | 49,96 % |
| `f100` | 2,645 V | 57 mV | 1,181 V | 118,1 % | 100,0 % |

| Parámetro | Nominal | Medido |
|---|---|---|
| Relación del divisor | 0,01768 | 0,02153 (+21,8 %; R2 = 213,9 Ω) |
| Ganancia del amplificador | 21,00 | 21,35 con R medidas; **21,24** real (R² = 0,9998) |
| Offset de salida | ±0,35 V aceptable | 0,015 V (0,70 mV referido a la entrada) |
| DAC | 0,01294 V por código | V1 = 0,01221 · n + 0,104 V |
| V0 (pin 3V3) | 3,30 V | 3,312 V |

```bash
python analysis.py
```

**Lo que se observó:**

- **TP1 salió por encima de su rango en 21, 50 y 100 %, y el error viene sobre todo del divisor.** R2 midió 213,9 Ω en lugar de 180 Ω: se colocó una resistencia de 220 Ω, que eleva la relación del divisor un 21,8 %. Con las resistencias medidas, el TP1 calculado a partir de V1 coincide con el medido dentro de 0,5 mV.
- **El DAC del ESP32 no es una fuente de precisión.** Entrega 0,108 V con el código 0 y su pendiente es un 6 % menor que la nominal. En el nivel de 21 % ese desplazamiento se suma al error del divisor.
- **El amplificador funcionó según su diseño.** El offset fue muy pequeño y la ganancia real quedó a +1,2 % de la de diseño y a −0,5 % de la calculada con las resistencias medidas.
- **TP2 quedó unos 2 mV de entrada por debajo de 21 × TP1 + offset en todos los niveles.** Es un desplazamiento constante, no un error de ganancia. Coincide con el desplazamiento del DAC multiplicado por la relación del divisor. Es posible que los valores de TP1 de la Tabla 4 se copiaran de la Tabla 2 sin volver a medirlos.
- **Sin calibrar, el canal lee de más**, y es el tipo de error más peligroso: una mezcla hipóxica de 18 % se leería como 22,4 % y la alarma de baja concentración no se activaría. La calibración de dos puntos corrige ganancia y offset a la vez: el nivel de 50 % se lee como 49,96 % y el de 0 % como 0,1 %, lo que confirma que el canal es lineal.
- **La señal fue estable.** En el osciloscopio, TP2 con `f50` fue una línea horizontal a 0,6 V, sin oscilaciones ni deriva.

**Conclusión.** La celda simulada y el amplificador funcionan; el error del canal proviene sobre todo de la resistencia R2 del divisor y del desplazamiento del DAC. Sin calibrar, el canal indicaría 25,9 % de O₂ en aire y 118,1 % en oxígeno puro. La calibración de dos puntos en aire y oxígeno puro corrige ese error, y por eso es obligatoria en los monitores de oxígeno de las máquinas de anestesia.

## Referencias

- Dorsch, J. A., y Dorsch, S. E. (2008). *Understanding Anesthesia Equipment* (5.ª ed.). Lippincott Williams & Wilkins.
- Garg, R., y Gupta, R. C. (2013). Analysis of oxygen, anaesthesia agent and flows in anaesthesia machine. *Indian Journal of Anaesthesia, 57*(5), 481–488. https://doi.org/10.4103/0019-5049.120144
- Lee, H.-C., et al. (2022). VitalDB, a high-fidelity multi-parameter vital signs database in surgical patients. *Scientific Data, 9*, 279. https://doi.org/10.1038/s41597-022-01411-5
- Espressif Systems. *Digital to Analog Converter (DAC) – ESP-IDF Programming Guide, ESP32*. https://docs.espressif.com/projects/esp-idf/en/v5.0.4/esp32/api-reference/peripherals/dac.html
- Texas Instruments. *TLx08x JFET-input operational amplifiers* (SLOS081) – Hoja de datos.
- International Organization for Standardization (2018). *ISO 80601-2-55:2018 – Respiratory gas monitors*.
- Pallás Areny, R., y Webster, J. G. (2001). *Sensors and Signal Conditioning* (2.ª ed.). John Wiley & Sons.
- Montgomery, D. C., y Runger, G. C. (2018). *Applied Statistics and Probability for Engineers* (7.ª ed.). John Wiley & Sons.
