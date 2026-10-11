/*
 * Taller de Equipos Biomédicos II - Guía de Laboratorio N.º 2
 * Caracterización de un sensor de presión manométrica con la Ley de Boyle
 * (MPS20N0040D-D + conversor HX710B, leído por ESP32)
 *
 * Cumple los 5 requisitos de la sección 6 de la guía:
 *  1. Lee el HX710B (protocolo tipo HX71x: SCK/DOUT) y entrega cada lectura
 *     como entero CON SIGNO (complemento a 2 de 24 bits).
 *  2. Promedia N lecturas consecutivas (N >= 20, constante durante la sesión),
 *     calcula media y desviación estándar -también con el puerto abierto al
 *     aire (nivel de cero)- y lo muestra por el Monitor Serial.
 *  3. Declara como constantes configurables Patm (kPa), V0 (mL) y Vm (mL)
 *     -valores provisionales de la sesión- y muestra la marca de parada
 *     Vj,min (ecuación 2) para Pmax = 35 kPa.
 *  4. Recibe por el Monitor Serial el ciclo, el sentido (compresión/
 *     descompresión) y el volumen Vj (mL, dos decimales), calcula Pref con
 *     la ecuación (1) y rechaza con advertencia todo Vj > V0, Vj <= 0, o
 *     cuyo Pref supere 40 kPa.
 *  5. Entrega una línea por punto, valores separados por comas:
 *     ciclo,sentido,Vj,Pref,media,desviación_estándar,N
 *
 * ================================ USO ================================
 * Por el Monitor Serial (115200 baudios, salto de línea "Nueva línea"):
 *
 *   Z                     -> promedia N lecturas con el puerto ABIERTO AL AIRE
 *                            (nivel de cero). Úsalo en la Etapa A (paso 4) y
 *                            otra vez al terminar el barrido (Etapa D, paso 20,
 *                            para obtener C0,f). Imprime: ZERO,media,desv,N
 *
 *   1,C,0.94              -> punto del barrido: ciclo 1, sentido Compresión,
 *                            Vj = 0.94 mL. Sentido también acepta D o
 *                            "descompresion" (sin tilde), en mayúsc./minúsc.
 *                            Imprime: ciclo,sentido,Vj,Pref,media,desv,N
 *
 * IMPORTANTE - antes de usarlo en la sesión real:
 *  - Verificar SCK y DOUT en el osciloscopio (paso 3 de la Etapa A) y
 *    confirmar contra la hoja de datos del HX710B que el número de pulsos
 *    de reloj y los tiempos en alto/bajo coinciden con lo que este código
 *    genera. El protocolo de bit-banging de abajo sigue el estándar de la
 *    familia HX710/HX711 (25 pulsos: 24 bits de dato + 1 pulso de cierre),
 *    que es lo habitual para este tipo de conversor, pero SIEMPRE se debe
 *    confirmar con la hoja de datos real y el osciloscopio, tal como pide
 *    la guía.
 *  - Actualizar PATM_KPA, V0_ML y VM_ML con los valores reales de tu sesión
 *    (paso 12): Patm = presión publicada, Vm = el menor de Va1 y Va2.
 * =======================================================================
 */

#include <math.h>

// ---------------- Pines (ajustar según el cableado real) ----------------
#define PIN_SCK   33
#define PIN_DOUT  32

// ---------------- Requisito 2: N de lecturas por promedio ----------------
// N >= 20 y debe permanecer CONSTANTE durante toda la sesión.
const int N_LECTURAS = 20;

// ---------------- Requisito 3: constantes configurables ----------------
// Valores PROVISIONALES de la sesión (paso 12). Actualizar antes de medir.
float PATM_KPA = 101.3f;   // presión atmosférica publicada (kPa)
float V0_ML    = 1.00f;    // volumen inicial del émbolo (mL)
float VM_ML    = 0.76f;    // volumen muerto provisional = min(Va1, Va2) (mL)
const float PMAX_KPA = 35.0f; // margen de seguridad (fijo por la guía)

// Rango del sensor, usado para rechazar Pref fuera de escala (requisito 4)
const float SENSOR_MAX_KPA = 40.0f;

float vjMinML = 0.0f; // se calcula en setup() con la ecuación (2)

// =======================================================================
// Lectura del HX710B: 24 bits en complemento a 2 -> entero con signo
// (protocolo de bit-banging estándar de la familia HX710/HX711)
// =======================================================================
long leerHX710() {
  // Esperar a que el módulo indique dato listo (DOUT en bajo)
  while (digitalRead(PIN_DOUT) == HIGH) {
    ; // espera activa
  }

  unsigned long valor = 0;

  // 24 pulsos de reloj: se lee 1 bit por flanco, MSB primero
  for (int i = 0; i < 24; i++) {
    digitalWrite(PIN_SCK, HIGH);
    delayMicroseconds(1);
    valor = (valor << 1) | digitalRead(PIN_DOUT);
    digitalWrite(PIN_SCK, LOW);
    delayMicroseconds(1);
  }

  // 25.º pulso: cierra la conversión y deja listo el canal para la
  // siguiente lectura (no aporta datos).
  digitalWrite(PIN_SCK, HIGH);
  delayMicroseconds(1);
  digitalWrite(PIN_SCK, LOW);
  delayMicroseconds(1);

  // Extender el signo: si el bit 23 (MSB de los 24) está en 1, el valor
  // es negativo -> restar 2^24 para obtener el entero con signo real.
  if (valor & 0x800000UL) {
    valor |= 0xFF000000UL;
  }

  return (long)valor;
}

// =======================================================================
// Promedia N_LECTURAS lecturas crudas y calcula media y desviación
// estándar muestral (denominador N-1, igual que STDEV.S en Excel).
// =======================================================================
void promediarLecturas(double &media, double &desviacion) {
  long lecturas[N_LECTURAS];
  double suma = 0.0;

  for (int i = 0; i < N_LECTURAS; i++) {
    lecturas[i] = leerHX710();
    suma += lecturas[i];
    delay(2); // pequeño respiro entre lecturas consecutivas
  }

  media = suma / N_LECTURAS;

  double sumaCuad = 0.0;
  for (int i = 0; i < N_LECTURAS; i++) {
    double d = lecturas[i] - media;
    sumaCuad += d * d;
  }
  desviacion = sqrt(sumaCuad / (N_LECTURAS - 1));
}

// =======================================================================
// Ecuación (2): marca de parada del émbolo
// Vj,min = (V0 - r * Vm) / (1 + r), con r = Pmax / Patm
// =======================================================================
float calcularVjMin(float patm, float v0, float vm, float pmax) {
  float r = pmax / patm;
  return (v0 - r * vm) / (1.0f + r);
}

// =======================================================================
// Ecuación (1): presión manométrica de referencia
// Pref = Patm * (V0 - Vj) / (Vj + Vm)
// =======================================================================
float calcularPref(float patm, float v0, float vj, float vm) {
  return patm * (v0 - vj) / (vj + vm);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  pinMode(PIN_SCK, OUTPUT);
  pinMode(PIN_DOUT, INPUT);
  digitalWrite(PIN_SCK, LOW);

  vjMinML = calcularVjMin(PATM_KPA, V0_ML, VM_ML, PMAX_KPA);

  Serial.println("=== Laboratorio 2 - Sensor de presión (HX710B + ESP32) ===");
  Serial.print("Patm = "); Serial.print(PATM_KPA, 2); Serial.println(" kPa");
  Serial.print("V0   = "); Serial.print(V0_ML, 2); Serial.println(" mL");
  Serial.print("Vm   = "); Serial.print(VM_ML, 2); Serial.println(" mL");
  Serial.print("Vj,min (marca de parada, Pmax=35 kPa) = ");
  Serial.print(vjMinML, 3); Serial.println(" mL");
  Serial.println();
  Serial.println("Comandos:");
  Serial.println("  Z                -> promedio a presion atmosferica (nivel de cero)");
  Serial.println("  ciclo,sentido,Vj -> punto del barrido, ej: 1,C,0.94");
  Serial.println();
}

void loop() {
  if (Serial.available() > 0) {
    String linea = Serial.readStringUntil('\n');
    linea.trim();
    if (linea.length() == 0) return;

    // ---------------- Comando Z: nivel de cero ----------------
    if (linea.equalsIgnoreCase("Z")) {
      double media, desviacion;
      promediarLecturas(media, desviacion);
      Serial.print("ZERO,");
      Serial.print(media, 2); Serial.print(",");
      Serial.print(desviacion, 2); Serial.print(",");
      Serial.println(N_LECTURAS);
      return;
    }

    // ---------------- Punto del barrido: ciclo,sentido,Vj ----------------
    int coma1 = linea.indexOf(',');
    int coma2 = linea.indexOf(',', coma1 + 1);
    if (coma1 < 0 || coma2 < 0) {
      Serial.println("ADVERTENCIA: formato no reconocido. Use  ciclo,sentido,Vj  o  Z");
      return;
    }

    String sCiclo   = linea.substring(0, coma1);
    String sSentido = linea.substring(coma1 + 1, coma2);
    String sVj      = linea.substring(coma2 + 1);

    int ciclo = sCiclo.toInt();
    float vj  = sVj.toFloat();

    sSentido.trim();
    String sentido;
    char s0 = toupper(sSentido.charAt(0));
    if (s0 == 'C') sentido = "Compresion";
    else if (s0 == 'D') sentido = "Descompresion";
    else {
      Serial.println("ADVERTENCIA: sentido no reconocido (use C o D).");
      return;
    }

    // ---------------- Validaciones (requisito 4) ----------------
    if (vj <= 0.0f || vj > V0_ML) {
      Serial.print("ADVERTENCIA: Vj fuera de rango (0 < Vj <= ");
      Serial.print(V0_ML, 2);
      Serial.println(" mL). Punto rechazado.");
      return;
    }

    float pref = calcularPref(PATM_KPA, V0_ML, vj, VM_ML);

    if (pref > SENSOR_MAX_KPA) {
      Serial.print("ADVERTENCIA: Pref calculada (");
      Serial.print(pref, 2);
      Serial.println(" kPa) supera el rango del sensor (40 kPa). Punto rechazado.");
      return;
    }

    // ---------------- Medición: promedio de N lecturas ----------------
    double media, desviacion;
    promediarLecturas(media, desviacion);

    // ---------------- Requisito 5: una línea por punto, separada por comas ----------------
    Serial.print(ciclo); Serial.print(",");
    Serial.print(sentido); Serial.print(",");
    Serial.print(vj, 2); Serial.print(",");
    Serial.print(pref, 3); Serial.print(",");
    Serial.print(media, 2); Serial.print(",");
    Serial.print(desviacion, 2); Serial.print(",");
    Serial.println(N_LECTURAS);
  }
}
