// GPS NEO-6M.
#pragma once
#include "bibliotecas.h"
#include "validacao.h"

namespace Gps {
#if USAR_GPS
  TinyGPSPlus dev;
  HardwareSerial porta(2);

  void iniciar() {
    porta.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
    Serial.println("GPS - aguardando fix...");
  }
  void ler() { while (porta.available() > 0) dev.encode(porta.read()); }
  void imprimir() {
    if (!dev.location.isValid()) { Serial.println("GPS: sem fix ainda"); return; }
    double lat = dev.location.lat(), lon = dev.location.lng();
    // Coordenada fora do globo, ou fix velho demais, nao e posicao confiavel.
    if (!Validacao::naFaixa((float)lat, -90.0f, 90.0f) || !Validacao::naFaixa((float)lon, -180.0f, 180.0f)) {
      Serial.printf("[VALIDACAO] GPS: coordenada invalida (%.6f, %.6f) - descartada\n", lat, lon);
      return;
    }
    if (dev.location.age() > GPS_IDADE_MAX_MS) {
      Serial.printf("[VALIDACAO] GPS: fix desatualizado (%lu ms) - sinal perdido\n", (unsigned long)dev.location.age());
      return;
    }
    Serial.printf("GPS: Lat=%.6f Lon=%.6f\n", lat, lon);
  }
#else
  inline void iniciar() {}
  inline void ler() {}
  inline void imprimir() {}
#endif
}
