"""
Taller de Equipos Biomédicos II – Laboratorio 3: canal de medición de O2.

Lee los CSV de data/ y calcula:
  - relación del divisor y ganancia del amplificador (nominal y con R medidas)
  - ajuste lineal de V1 frente al código DAC
  - ganancia real: pendiente de TP2 frente a TP1 (mínimos cuadrados)
  - FiO2 indicada sin calibrar (10 mV por %) y con calibración de dos puntos
"""

import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def leer(nombre):
    with open(ROOT / "data" / nombre, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def ajuste(x, y):
    mx, my = sum(x) / len(x), sum(y) / len(y)
    sxy = sum((a - mx) * (b - my) for a, b in zip(x, y))
    sxx = sum((a - mx) ** 2 for a in x)
    m = sxy / sxx
    b = my - m * mx
    ss_res = sum((c - (m * a + b)) ** 2 for a, c in zip(x, y))
    ss_tot = sum((c - my) ** 2 for c in y)
    return m, b, 1 - ss_res / ss_tot


def main():
    r = {f["resistencia"]: (float(f["nominal_ohm"]), float(f["medido_ohm"]))
         for f in leer("resistencias.csv")}
    med = leer("mediciones.csv")
    cond = {f["medicion"]: float(f["valor"]) for f in leer("condiciones.csv")}
    offset = cond["Offset de salida (entrada a GND)"]

    def divisor(i):
        return r["R2"][i] / (r["R1"][i] + r["R2"][i])

    def ganancia(i):
        return 1 + (r["Rf1"][i] + r["Rf2"][i]) / r["Rg"][i]

    print(f"Relación del divisor   nominal {divisor(0):.5f}   medida {divisor(1):.5f}"
          f"   ({(divisor(1) / divisor(0) - 1) * 100:+.1f} %)")
    print(f"Ganancia del amplif.   nominal {ganancia(0):.2f}     medida {ganancia(1):.2f}")

    codigo = [float(f["codigo_dac"]) for f in med]
    v1 = [float(f["v1_medido_V"]) for f in med]
    m, b, _ = ajuste(codigo, v1)
    print(f"\nDAC: V1 = {m:.5f} · n + {b:.3f} V   (nominal {3.3 / 255:.5f} V por código)")

    tp1 = [float(f["tp1_medido_mV"]) / 1000 for f in med]
    tp2 = [float(f["tp2_medido_V"]) for f in med]
    g, b2, r2 = ajuste(tp1, tp2)
    print(f"\nGanancia real (pendiente TP2 vs TP1): {g:.2f}   ordenada {b2:+.3f} V   R² = {r2:.4f}")
    print(f"Offset referido a la entrada: {offset / ganancia(1) * 1000:.2f} mV")

    v21 = tp2[[f["comando"] for f in med].index("f21")]
    v100 = tp2[[f["comando"] for f in med].index("f100")]
    print(f"\n{'Comando':<8}{'TP2 (V)':>9}{'FiO2 sin calibrar':>20}{'FiO2 calibrada':>17}")
    for f, v in zip(med, tp2):
        sin_cal = v / 0.010
        cal = 21 + (v - v21) * (100 - 21) / (v100 - v21)
        print(f"{f['comando']:<8}{v:>9.3f}{sin_cal:>18.1f} %{cal:>15.2f} %")


if __name__ == "__main__":
    main()
