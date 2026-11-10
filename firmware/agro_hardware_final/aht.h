// Sensor AHT10 (temperatura ambiente e umidade).
#pragma once
#include "bibliotecas.h"
#include "estado.h"
#include "validacao.h"

namespace Aht {
#if USAR_AHT
  Adafruit_AHTX0 dev;
  bool ok = false;
  bool leituraOk = false;   // ultima leitura valida e sensor fora de falha
  unsigned long contInj = 0;
  Validacao::Canal canalTemp = Validacao::novoCanal("AHT10 temperatura");
  Validacao::Canal canalUmid = Validacao::novoCanal("AHT10 umidade");

  void iniciar() {
    if (!dev.begin()) { Serial.println("ERRO: AHT10 nao encontrado! (0x38 no scanner?)"); return; }
    ok = true;
    Serial.println("AHT10 OK");
  }
  void ler() {
    if (!ok) return;
    sensors_event_t umid, temp;
    dev.getEvent(&umid, &temp);
    float t = Validacao::injetar(temp.temperature, contInj);
    float u = umid.relative_humidity;

    // Faixa do componente + salto plausivel desde a ultima leitura aceita.
    bool tOk = Validacao::naFaixa(t, AHT_TEMP_MIN, AHT_TEMP_MAX) &&
               (canalTemp.aceitas == 0 || canalTemp.emFalha || Validacao::saltoPlausivel(Estado.tempAmbiente, t, AHT_SALTO_TEMP_MAX));
    bool uOk = Validacao::naFaixa(u, AHT_UMID_MIN, AHT_UMID_MAX);

    Validacao::avisar(canalTemp, Validacao::registrar(canalTemp, tOk, FALHAS_SEGUIDAS_ERRO), t, "-40..85 C / salto 15 C");
    Validacao::avisar(canalUmid, Validacao::registrar(canalUmid, uOk, FALHAS_SEGUIDAS_ERRO), u, "0..100 %UR");

    if (tOk) Estado.tempAmbiente = t;
    if (uOk) Estado.umidadeAr = u;
    leituraOk = !canalTemp.emFalha && !canalUmid.emFalha && canalTemp.aceitas > 0 && canalUmid.aceitas > 0;
  }
  void imprimir() {
    if (!ok) { Serial.println("AHT10 indisponivel (falhou no setup)"); return; }
    if (!leituraOk) { Serial.println("Ambiente: SEM LEITURA VALIDA (sensor em falha - veja [VALIDACAO])"); return; }
    Serial.printf("Ambiente: %.1f C | %.1f %% UR\n", Estado.tempAmbiente, Estado.umidadeAr);
  }
  void telemetria(Json &j) {
    if (!ok || !leituraOk) return;   // sem leitura valida: campo omitido (NULL no banco)
    j.add(",\"temp_ambiente\":%.1f,\"umidade_ar\":%.1f", Estado.tempAmbiente, Estado.umidadeAr);
  }
#else
  inline void iniciar() {}
  inline void ler() {}
  inline void imprimir() {}
  inline void telemetria(Json&) {}
#endif
}
