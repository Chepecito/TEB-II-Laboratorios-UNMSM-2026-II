# TEB II – Laboratorio 3: Simulador de celda de oxígeno (FiO₂) con ESP32

Taller de Equipos Biomédicos II · Escuela Profesional de Ingeniería Biomédica · UNMSM

*(En preparación: se agregarán el esquemático, los datos medidos, el informe y los resultados.)*

## Estructura

```
firmware/TBIII_LABO_3_CODIGO/   Sketch de Arduino usado en el laboratorio
```

## Firmware

[`TBIII_LABO_3_CODIGO.ino`](firmware/TBIII_LABO_3_CODIGO/TBIII_LABO_3_CODIGO.ino) usa el DAC del ESP32 (GPIO25) para simular la salida de una celda galvánica de oxígeno, cuya tensión es proporcional a la fracción inspirada de oxígeno (FiO₂).

- **Sensibilidad simulada:** 10 mV a 21 % de O₂ (≈ 0,476 mV por punto porcentual).
- **Divisor de salida:** R1 = 10 kΩ y R2 = 180 Ω reducen la salida del DAC (0–3,3 V) al rango de milivoltios de una celda real. El código calcula el valor del DAC dividiendo la tensión de la celda entre el factor del divisor, R2 / (R1 + R2) ≈ 0,0177.
- **Modo fijo:** arranca entregando FiO₂ = 21 %. Con `f<valor>` se fija cualquier FiO₂ entre 0 y 100 %.
- **Modo reproducción:** con `r` reproduce una serie de 504 valores de FiO₂ (35–96 %) del caso 95 de la base de datos VitalDB, una muestra cada 0,5 s (unas 14 veces más rápido que el registro original; el recorrido completo dura unos 4 min 12 s). Con `p` se pausa y se reanuda.
- Por el Monitor Serial muestra la FiO₂, la tensión teórica de la celda y el código del DAC.

| FiO₂ | Tensión de la celda | Salida del DAC (antes del divisor) | Código DAC |
|---|---|---|---|
| 21 % | 10,00 mV | 0,566 V | 44 |
| 35 % | 16,67 mV | 0,943 V | 73 |
| 95 % | 45,24 mV | 2,558 V | 198 |
| 100 % | 47,62 mV | 2,693 V | 208 |

Monitor Serial a **115200 baudios**, terminación *Nueva línea*.

| Comando | Acción |
|---|---|
| `r` | Reproduce el caso 95 de VitalDB desde el inicio |
| `p` | Pausa o reanuda la reproducción |
| `f<valor>` | Fija la FiO₂, por ejemplo `f50` para 50 % |
