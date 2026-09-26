"""Análisis de calibración - TEB II Lab 1.

Lee data/tabla1_calibracion.csv, calcula el error porcentual de cada salida
(DAC y PWM filtrado) respecto al voltaje programado, la resolución efectiva y
genera la curva de calibración en docs/curva_calibracion.png.

Uso:  python analysis.py
"""
import csv
import sys
from pathlib import Path

import numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).parent
CSV = ROOT / "data" / "tabla1_calibracion.csv"
OUT = ROOT / "docs" / "curva_calibracion.png"
VREF = 3.3
CODES = 255  # 2^8 - 1


def load():
    rows = []
    with open(CSV, newline="", encoding="utf-8") as f:
        for r in csv.DictReader(f):
            try:
                rows.append(tuple(float(r[k]) for k in
                            ("v_programado_V", "v_medido_dac_V", "v_medido_pwm_V")))
            except ValueError:
                continue  # fila incompleta
    if not rows:
        sys.exit("No hay datos completos en el CSV. Complete la Tabla 1 primero.")
    return np.array(rows)


def pct_error(prog, med):
    """Error % respecto al valor programado (indefinido si prog = 0)."""
    with np.errstate(divide="ignore", invalid="ignore"):
        return np.where(prog != 0, (med - prog) / prog * 100.0, np.nan)


def main():
    d = load()
    vp, vd, vw = d[:, 0], d[:, 1], d[:, 2]
    e_dac, e_pwm = pct_error(vp, vd), pct_error(vp, vw)

    print(f"{'V prog':>7} {'V DAC':>7} {'V PWM':>7} {'Err DAC %':>10} {'Err PWM %':>10}")
    for row in zip(vp, vd, vw, e_dac, e_pwm):
        print("{:7.3f} {:7.3f} {:7.3f} {:10.2f} {:10.2f}".format(*row))

    # Sensibilidad (pendiente) y offset por regresión lineal medido vs programado
    for nombre, v in (("DAC", vd), ("PWM filtrado", vw)):
        m, b = np.polyfit(vp, v, 1)
        print(f"{nombre}: sensibilidad = {m:.4f} V/V, offset = {b*1000:.1f} mV")

    print(f"Resolución teórica: Vref/{CODES} = {VREF / CODES * 1000:.2f} mV/código")
    print("Nota: la resolución efectiva se estima a partir del voltaje medido "
          "por código; ver README.")

    fig, ax = plt.subplots(figsize=(6, 5))
    ax.plot([0, VREF], [0, VREF], "k--", lw=1, label="Ideal (y = x)")
    ax.plot(vp, vd, "o-", label="DAC (GPIO25)")
    ax.plot(vp, vw, "s-", label="PWM filtrado (GPIO27)")
    ax.set_xlabel("Voltaje programado (V)")
    ax.set_ylabel("Voltaje medido (V)")
    ax.set_title("Curva de calibración - ESP32")
    ax.grid(alpha=0.3)
    ax.legend()
    OUT.parent.mkdir(exist_ok=True)
    fig.tight_layout()
    fig.savefig(OUT, dpi=200)
    print(f"Gráfica guardada en {OUT}")


if __name__ == "__main__":
    main()
