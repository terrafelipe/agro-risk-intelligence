// Sensor de chama KY-026 (A0 analogico).
#pragma once
#include "config.h"
#include "estado.h"
#include "evento.h"
#include "validacao.h"

namespace Chama {
#if USAR_CHAMA
  bool anterior = false;
  int  leituraRaw = 4095;   // ultimo A0 lido (0-4095); alto = sem chama
  unsigned long contInj = 0;
  Validacao::Canal canal = Validacao::novoCanal("Sensor de chama KY-026");

  // O D0 (digital) deste KY-026 queimou numa inversao de VCC/GND - fica travado.
  // O estagio analogico (A0) sobreviveu: o raw CAI em direcao a 0 quanto mais
  // perto a chama. Lemos A0 no GPIO35 (ADC1, que aguenta o Wi-Fi; o antigo D0/25
  // era ADC2 e o ADC2 morre com o radio ligado) e aplicamos LIMIAR_CHAMA por
  // software. GPIO34 e so-de-entrada: nao precisa de pinMode.
  void iniciar() {}
  void ler() {
    int raw = (int)Validacao::injetar((float)analogRead(CHAMA_PIN), contInj);
    // 4095 = pino sem sinal (fio solto, satura em cima); fora de 0..4094 nao e leitura de chama.
    bool valida = Validacao::naFaixa((float)raw, CHAMA_RAW_MIN, CHAMA_RAW_MAX);
    Validacao::avisar(canal, Validacao::registrar(canal, valida, FALHAS_SEGUIDAS_ERRO), (float)raw, "0..4094 (ADC)");
    if (!valida) return;   // mantem a ultima leitura e a ultima decisao validas
    leituraRaw = raw;
    Estado.chamaDetectada = leituraRaw < LIMIAR_CHAMA;   // perto da chama = raw baixo
  }
  void eventos() {
    if (Estado.chamaDetectada && !anterior) {
      char det[48];
      snprintf(det, sizeof(det), "{\"sensor\":\"ky026\",\"a0\":%d}", leituraRaw);
      Evento::disparar("INCENDIO", "chama detectada", "chama_detectada", 5, det);
    }
    anterior = Estado.chamaDetectada;
  }
  // Imprime o raw SEMPRE: e assim que se calibra o LIMIAR_CHAMA - anote o valor
  // parado (baseline) e o valor com a chama na distancia desejada; ponha o limiar no meio.
  void imprimir() {
    Serial.printf("Chama: A0=%d (limiar %d)%s\n", leituraRaw, LIMIAR_CHAMA,
                  Estado.chamaDetectada ? "  <-- CHAMA DETECTADA" : "");
  }
  void telemetria(Json &j) {
    j.add(",\"chama_detectada\":%s", Estado.chamaDetectada ? "true" : "false");
  }
#else
  inline void iniciar() {}
  inline void ler() {}
  inline void eventos() {}
  inline void imprimir() {}
  inline void telemetria(Json&) {}
#endif
}
