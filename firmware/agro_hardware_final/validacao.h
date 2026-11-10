// Validacao das leituras: descarta valor invalido ou incoerente, conta as falhas
// seguidas de cada sensor e avisa no Monitor Serial.
//
// Regra de ouro do projeto: leitura invalida NUNCA vira dado. Ela e descartada, o
// sistema mantem o ultimo valor valido, e se o sensor continuar falhando o campo
// some da telemetria (coluna NULL no banco) em vez de gravar uma medicao inventada.
//
// Este arquivo nao depende de bibliotecas do Arduino (so o aviso no Serial, dentro
// de #ifdef ARDUINO), por isso as regras tambem rodam no computador:
//   g++ firmware/tests/test_validacao.cpp -o test_validacao && ./test_validacao
#pragma once

namespace Validacao {

  // NaN nao e igual a si mesmo; o limite largo barra infinito e lixo de leitura.
  inline bool numeroValido(float v) { return v == v && v > -1.0e6f && v < 1.0e6f; }

  // Limites minimo e maximo FISICOS do sensor (nao os limiares de alerta).
  inline bool naFaixa(float v, float minimo, float maximo) {
    return numeroValido(v) && v >= minimo && v <= maximo;
  }

  // Salto maximo entre duas leituras seguidas (coerencia no tempo).
  inline bool saltoPlausivel(float anterior, float atual, float maximo) {
    float d = atual - anterior;
    if (d < 0) d = -d;
    return d <= maximo;
  }

  // Um canal por grandeza medida.
  //   seguidas   - leituras invalidas em sequencia (zera na primeira valida)
  //   rejeitadas - total descartado desde que ligou
  //   aceitas    - total aproveitado
  //   emFalha    - liga quando 'seguidas' chega ao limite: sensor suspeito
  struct Canal {
    const char* nome;
    unsigned seguidas;
    unsigned long rejeitadas;
    unsigned long aceitas;
    bool emFalha;
  };

  inline Canal novoCanal(const char* nome) {
    Canal c = {nome, 0, 0, 0, false};
    return c;
  }

  enum Resultado { ACEITA, REJEITADA, FALHA_NOVA, RECUPERADA };

  // Registra o veredito de uma leitura e diz o que mudou no estado do canal.
  inline Resultado registrar(Canal& c, bool valida, unsigned limiteSeguidas) {
    if (valida) {
      c.aceitas++;
      c.seguidas = 0;
      if (c.emFalha) { c.emFalha = false; return RECUPERADA; }
      return ACEITA;
    }
    c.rejeitadas++;
    c.seguidas++;
    if (!c.emFalha && c.seguidas >= limiteSeguidas) { c.emFalha = true; return FALHA_NOVA; }
    return REJEITADA;
  }

  // Demo: com DIAG_INJETAR_INVALIDO 1 no config.h, a cada 7a leitura o valor real e
  // trocado por um absurdo (9999) - e assim que se mostra a validacao funcionando.
  inline float injetar(float v, unsigned long& n) {
#if defined(DIAG_INJETAR_INVALIDO) && DIAG_INJETAR_INVALIDO
    if (++n % 7 == 0) return 9999.0f;
#else
    (void)n;
#endif
    return v;
  }

#ifdef ARDUINO
  // Mensagens de erro no Monitor Serial (so quando algo muda ou e descartado).
  inline void avisar(const Canal& c, Resultado r, float valor, const char* faixa) {
    switch (r) {
      case REJEITADA:
        Serial.printf("[VALIDACAO] %s: leitura invalida (%.2f), fora de %s - descartada, mantido o ultimo valor valido\n",
                      c.nome, valor, faixa);
        break;
      case FALHA_NOVA:
        Serial.printf("[VALIDACAO] ERRO: %s com %u leituras invalidas seguidas - confira fiacao e sensor\n",
                      c.nome, c.seguidas);
        break;
      case RECUPERADA:
        Serial.printf("[VALIDACAO] %s voltou a ler valores validos\n", c.nome);
        break;
      default:
        break;
    }
  }
#endif

}  // namespace Validacao
