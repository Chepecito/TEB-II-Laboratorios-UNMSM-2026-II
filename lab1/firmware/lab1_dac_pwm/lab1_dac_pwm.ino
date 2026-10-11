/*
 * Taller de Equipos Biomédicos II - Laboratorio N.º 1
 * Generación y calibración de una señal analógica con ESP32
 * UNMSM - EAP Ingeniería Biomédica
 *
 * Lee un voltaje objetivo (0 a 3.3 V) por el Monitor Serial, lo convierte a un
 * código de 8 bits (0-255) y lo entrega simultáneamente por:
 *   - DAC interno de 8 bits en GPIO25  (Vref ~ 3.3 V)
 *   - PWM (periférico LEDC) en GPIO27, 5000 Hz, 8 bits -> requiere filtro RC
 *     pasabajos (R = 10 kOhm, C = 1 uF, fc = 1/(2*pi*R*C) ~ 15.9 Hz)
 *
 * Ambas salidas usan la misma escala 0-255, por lo que son directamente
 * comparables.
 *
 * Requiere el core Arduino-ESP32 3.x (API ledcAttach). Monitor Serial a
 * 115200 baudios, con terminación "Nueva línea".
 */

const uint8_t PIN_DAC = 25;        // DAC1 (real, 8 bits)
const uint8_t PIN_PWM = 27;        // Salida PWM (LEDC)
const uint32_t PWM_FREQ_HZ = 5000; // Frecuencia portadora
const uint8_t PWM_RES_BITS = 8;    // Resolución del PWM
const float V_MAX = 3.3f;          // Vref aproximado del DAC / VDD
const uint16_t CODE_MAX = 255;     // 2^8 - 1

void setup() {
  Serial.begin(115200);
  ledcAttach(PIN_PWM, PWM_FREQ_HZ, PWM_RES_BITS);
  dacWrite(PIN_DAC, 0);
  ledcWrite(PIN_PWM, 0);

  Serial.println();
  Serial.println("=== TEB II - Lab 1: DAC vs PWM filtrado ===");
  Serial.println("Ingrese un voltaje objetivo entre 0 y 3.3 V y presione Enter.");
}

void loop() {
  if (Serial.available() == 0) return;

  float v = Serial.parseFloat();
  // Descartar el resto de la línea (salto de línea, espacios)
  while (Serial.available() > 0) Serial.read();

  // Limitar al rango válido del DAC
  bool recortado = false;
  if (v < 0.0f)   { v = 0.0f;  recortado = true; }
  if (v > V_MAX)  { v = V_MAX; recortado = true; }

  // Voltaje -> código de 8 bits (0-255)
  uint8_t code = (uint8_t)roundf(v / V_MAX * CODE_MAX);

  // Misma escala 0-255 para ambas salidas
  dacWrite(PIN_DAC, code);
  ledcWrite(PIN_PWM, code);

  float v_teorico = (float)code * V_MAX / CODE_MAX;

  Serial.printf("V ingresado: %.3f V%s | Codigo: %u | V teorico con ese codigo: %.3f V\n",
                v, recortado ? " (recortado al rango)" : "", code, v_teorico);
}
