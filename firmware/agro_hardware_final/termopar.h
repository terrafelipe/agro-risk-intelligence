// Termopar MAX6675 (temperatura do escape).
#pragma once
#include "bibliotecas.h"
#include "estado.h"
#include "evento.h"
#include "validacao.h"

namespace Termopar {
#if USAR_TERMOPAR
  // Lib RobTillaart: construtor (select, miso, clock) - CS primeiro, nao o clock.
  // Pinos PROPRIOS (13/4), fora do SPI do RC522: o modulo MAX6675 nao libera a
  // linha MISO quando nao esta selecionado e travava a leitura do cartao.
  MAX6675 dev(TERMOPAR_CS, TERMOPAR_SO, TERMOPAR_SCK);
  bool ok = false;
  unsigned long contInj = 0;
  Validacao::Canal canal = Validacao::novoCanal("Termopar MAX6675");
  bool atencaoAnterior = false, criticoAnterior = false;

  void iniciar() { dev.begin(); Serial.println("MAX6675 iniciado"); }

  void ler() {
    // 1) falha de comunicacao: o chip avisa (fio solto, CS errado, termopar aberto).
    int status = dev.read();
    bool valida = (status == STATUS_OK);
    float c = 0;
    if (!valida) {
      Serial.printf("[VALIDACAO] Termopar MAX6675: falha de leitura (status %d) - confira fiacao/CS/termopar\n", status);
    } else {
      // 2) faixa fisica do MAX6675 e 3) salto incoerente desde a ultima leitura aceita.
      c = Validacao::injetar(dev.getCelsius(), contInj);
      valida = Validacao::naFaixa(c, TERMOPAR_MIN, TERMOPAR_MAX) &&
               (canal.aceitas == 0 || canal.emFalha || Validacao::saltoPlausivel(Estado.tempEscape, c, TERMOPAR_SALTO_MAX));
      if (!valida) Validacao::avisar(canal, Validacao::REJEITADA, c, "0..1023,75 C / salto 150 C");
    }
    Validacao::Resultado r = Validacao::registrar(canal, valida, FALHAS_SEGUIDAS_ERRO);
    if (r == Validacao::FALHA_NOVA || r == Validacao::RECUPERADA) Validacao::avisar(canal, r, c, "");
    if (valida) Estado.tempEscape = c;      // invalida: mantem o ultimo valor valido
    ok = valida || (!canal.emFalha && canal.aceitas > 0);   // falha prolongada tira o campo da telemetria
  }

  void eventos() {
    bool critico = Estado.tempEscape >= TEMP_ESCAPE_CRITICO;
    bool atencao = Estado.tempEscape >= TEMP_ESCAPE_ATENCAO;
    char det[64];
    if (critico && !criticoAnterior) {
      snprintf(det, sizeof(det), "{\"limiar_c\":%.0f}", (float)TEMP_ESCAPE_CRITICO);
      Evento::disparar("INCENDIO", "temperatura do escape critica", "escape_critico", 4, det);
    } else if (atencao && !critico && !atencaoAnterior) {
      snprintf(det, sizeof(det), "{\"limiar_c\":%.0f}", (float)TEMP_ESCAPE_ATENCAO);
      Evento::disparar("INCENDIO", "temperatura do escape em atencao", "escape_atencao", 2, det);
    }
    criticoAnterior = critico;
    atencaoAnterior = atencao;
  }

  void imprimir() {
    if (ok) Serial.printf("Termopar: %.1f C (atencao %.0f, critico %.0f)\n", Estado.tempEscape,
                          (float)TEMP_ESCAPE_ATENCAO, (float)TEMP_ESCAPE_CRITICO);
    else    Serial.println("Termopar: SEM LEITURA VALIDA (sensor em falha - veja [VALIDACAO])");
  }
  void telemetria(Json &j) { if (ok) j.add(",\"temp_escape\":%.1f", Estado.tempEscape); }
#else
  const bool ok = false;
  inline void iniciar() {}
  inline void ler() {}
  inline void eventos() {}
  inline void imprimir() {}
  inline void telemetria(Json&) {}
#endif
}
