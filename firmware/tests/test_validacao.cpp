// Teste das regras de validacao no computador (sem ESP32):
//   g++ -std=c++11 firmware/tests/test_validacao.cpp -o /tmp/test_validacao && /tmp/test_validacao
#include <cstdio>
#include <cmath>
#include <limits>
#include "../agro_hardware_final/validacao.h"
#include "../agro_hardware_final/config.h"

static int falhas = 0, total = 0;
#define CHECK(c) do { total++; if (!(c)) { falhas++; printf("FALHOU: %s (linha %d)\n", #c, __LINE__); } } while (0)

int main() {
  using namespace Validacao;
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const float inf = std::numeric_limits<float>::infinity();

  // numero valido / NaN / infinito
  CHECK(numeroValido(25.0f));
  CHECK(!numeroValido(nan));
  CHECK(!numeroValido(inf));
  CHECK(!numeroValido(-inf));

  // limites minimo e maximo (AHT10: -40..85 C, 0..100 %UR)
  CHECK(naFaixa(25.0f, AHT_TEMP_MIN, AHT_TEMP_MAX));
  CHECK(naFaixa(-40.0f, AHT_TEMP_MIN, AHT_TEMP_MAX));      // borda inclusiva
  CHECK(naFaixa(85.0f, AHT_TEMP_MIN, AHT_TEMP_MAX));
  CHECK(!naFaixa(-40.1f, AHT_TEMP_MIN, AHT_TEMP_MAX));
  CHECK(!naFaixa(85.1f, AHT_TEMP_MIN, AHT_TEMP_MAX));
  CHECK(!naFaixa(130.0f, AHT_UMID_MIN, AHT_UMID_MAX));     // umidade > 100 %
  CHECK(!naFaixa(-3.0f, AHT_UMID_MIN, AHT_UMID_MAX));
  CHECK(!naFaixa(nan, AHT_UMID_MIN, AHT_UMID_MAX));

  // termopar MAX6675: 0..1023,75 C
  CHECK(naFaixa(45.0f, TERMOPAR_MIN, TERMOPAR_MAX));
  CHECK(!naFaixa(1500.0f, TERMOPAR_MIN, TERMOPAR_MAX));
  CHECK(!naFaixa(-5.0f, TERMOPAR_MIN, TERMOPAR_MAX));

  // chama: ADC 0..4094 (4095 = fio solto)
  CHECK(naFaixa(1962.0f, CHAMA_RAW_MIN, CHAMA_RAW_MAX));
  CHECK(!naFaixa(4095.0f, CHAMA_RAW_MIN, CHAMA_RAW_MAX));

  // MPU: magnitude ate 80 m/s2
  CHECK(naFaixa(9.8f, 0.0f, ACEL_MAG_MAX));
  CHECK(!naFaixa(300.0f, 0.0f, ACEL_MAG_MAX));

  // salto incoerente
  CHECK(saltoPlausivel(25.0f, 26.0f, AHT_SALTO_TEMP_MAX));
  CHECK(saltoPlausivel(25.0f, 10.0f, AHT_SALTO_TEMP_MAX));  // 15 = limite, aceita
  CHECK(!saltoPlausivel(25.0f, 60.0f, AHT_SALTO_TEMP_MAX));
  CHECK(!saltoPlausivel(60.0f, 25.0f, AHT_SALTO_TEMP_MAX)); // queda brusca tambem

  // contagem de falhas seguidas, falha do sensor e recuperacao
  Canal c = novoCanal("teste");
  CHECK(registrar(c, true, 3) == ACEITA);
  CHECK(registrar(c, false, 3) == REJEITADA);
  CHECK(registrar(c, false, 3) == REJEITADA);
  CHECK(!c.emFalha);
  CHECK(registrar(c, false, 3) == FALHA_NOVA);               // 3a seguida
  CHECK(c.emFalha);
  CHECK(registrar(c, false, 3) == REJEITADA);                // nao repete o aviso de falha
  CHECK(registrar(c, true, 3) == RECUPERADA);
  CHECK(!c.emFalha && c.seguidas == 0);
  CHECK(c.aceitas == 2 && c.rejeitadas == 4);

  // uma leitura boa no meio zera a sequencia: falha isolada nao derruba o sensor
  Canal d = novoCanal("isolada");
  registrar(d, false, 3); registrar(d, false, 3); registrar(d, true, 3); registrar(d, false, 3);
  CHECK(!d.emFalha && d.seguidas == 1);

  printf("%d verificacoes, %d falha(s)\n", total, falhas);
  return falhas ? 1 : 0;
}
