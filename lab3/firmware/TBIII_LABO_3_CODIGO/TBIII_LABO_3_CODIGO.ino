const int PIN_DAC = 25;
const float MV_POR_PORCIENTO = 10.0 / 21.0;      // sensibilidad de la celda simulada
const float R1 = 10000.0, R2 = 180.0;              // divisor de salida
const float K_DIV = R2 / (R1 + R2);
const unsigned long PERIODO_MS = 500;              // 0.5 s por muestra (~14x)

const uint8_t FIO2_CASO95[] = {
  88, 88, 88, 95, 96, 95, 94, 94, 94, 94, 94, 94, 94, 94, 94, 94, 95, 94, 93, 93,
  93, 93, 93, 93, 93, 93, 93, 93, 93, 92, 93, 93, 93, 93, 92, 93, 93, 93, 93, 94,
  94, 94, 94, 94, 89, 92, 94, 93, 93, 93, 93, 94, 94, 94, 94, 94, 94, 94, 94, 94,
  94, 94, 92, 92, 92, 92, 92, 93, 92, 92, 93, 93, 94, 93, 94, 91, 90, 88, 86, 84,
  83, 80, 79, 77, 76, 74, 73, 72, 71, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59,
  59, 58, 57, 57, 56, 55, 55, 54, 54, 53, 52, 52, 52, 51, 50, 50, 50, 49, 49, 48,
  48, 47, 47, 47, 47, 46, 46, 46, 45, 45, 45, 45, 44, 44, 44, 43, 43, 43, 43, 44,
  44, 43, 43, 42, 42, 42, 42, 42, 42, 41, 41, 41, 41, 41, 41, 40, 41, 40, 40, 40,
  40, 40, 39, 40, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 38, 38, 38,
  38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 38, 37, 37, 38, 37, 37, 37, 37, 37,
  37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
  37, 36, 37, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
  36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
  36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
  36, 36, 36, 36, 36, 36, 36, 36, 36, 37, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
  36, 35, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 35, 36, 36, 35, 35, 35, 36, 35,
  35, 36, 35, 35, 35, 35, 35, 35, 35, 35, 36, 35, 35, 35, 35, 35, 35, 35, 35, 35,
  35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
  35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
  35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
  35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
  35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
  35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 35, 48, 58, 69, 78, 84, 87, 90, 92, 93,
  88, 94, 94, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 96, 94, 95, 95, 95, 95,
  95, 95, 95, 95, 95, 96, 95, 94, 96, 95, 95, 95, 95, 95, 95, 94, 85, 85, 85, 85,
  95, 95, 95, 95
};
const int N = sizeof(FIO2_CASO95);

enum Modo { REPRODUCIR, FIJO };
Modo modo = FIJO;                                  // arranca fijo en 21 %
bool enPausa = false;
int indice = 0;
unsigned long tAnterior = 0;

uint8_t codigoDAC(float fio2) {
  float mv = fio2 * MV_POR_PORCIENTO;
  float v = (mv / 1000.0) / K_DIV;
  int c = (int)lround(v / 3.3 * 255.0);
  return (uint8_t)constrain(c, 0, 255);
}

void escribir(float fio2) {
  uint8_t c = codigoDAC(fio2);
  dacWrite(PIN_DAC, c);
  Serial.printf("FiO2 = %5.1f %%   Vcelda teorica = %5.2f mV   codigo DAC = %3d\n",
                fio2, fio2 * MV_POR_PORCIENTO, c);
}

void leerComando() {
  if (!Serial.available()) return;
  String s = Serial.readStringUntil('\n');
  s.trim();
  if (s.length() == 0) return;
  char c = tolower(s[0]);
  if (c == 'r') {
    modo = REPRODUCIR; enPausa = false; indice = 0;
    Serial.println(">> Reproduciendo caso 95 de VitalDB");
  } else if (c == 'p' && modo == REPRODUCIR) {
    enPausa = !enPausa;
    Serial.println(enPausa ? ">> Pausa" : ">> Reanudado");
  } else if (c == 'f') {
    float v = s.substring(1).toFloat();
    if (v < 0 || v > 100) { Serial.println(">> Valor fuera de rango (0-100)"); return; }
    modo = FIJO;
    Serial.print(">> FiO2 fija: ");
    escribir(v);
  } else {
    Serial.println(">> Comandos: r | p | f<valor>");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Simulador de celda de O2 — comandos: r | p | f<valor>");
  Serial.print(">> Inicio con FiO2 fija: ");
  escribir(21);
}

void loop() {
  leerComando();
  if (modo == REPRODUCIR && !enPausa && millis() - tAnterior >= PERIODO_MS) {
    tAnterior = millis();
    Serial.printf("[%3d/%d] ", indice + 1, N);
    escribir(FIO2_CASO95[indice]);
    indice = (indice + 1) % N;
  }
}