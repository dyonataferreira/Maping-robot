#pragma once
#include "variaveis.h"

void atualizarOdometria() {
  uint32_t tempoAtual = millis();
  float dt = (tempoAtual - ultimoTempoOdom) / 1000.0;
  ultimoTempoOdom = tempoAtual;

  if (dt > 0.5) return;

  float avgVel = (velE_global + velD_global) / 200.0;
  float speed_cm_s = avgVel * fatorOdometria;

  roboX_cm += speed_cm_s * cos(roboTheta) * dt;
  roboY_cm += speed_cm_s * sin(roboTheta) * dt;

  if (roboX_cm < 0) roboX_cm = 0;
  if (roboX_cm >= 499) roboX_cm = 499;
  if (roboY_cm < 0) roboY_cm = 0;
  if (roboY_cm >= 499) roboY_cm = 499;

  roboX = (int)(roboX_cm / 5.0);
  roboY = (int)(roboY_cm / 5.0);

  // --- A BORRACHA DINÂMICA (Clear Footprint) ---
  if (xSemaphoreTake(xMutexMapa, pdMS_TO_TICKS(5)) == pdTRUE) {
    for (int i = -raioBorracha; i <= raioBorracha; i++) {
      for (int j = -raioBorracha; j <= raioBorracha; j++) {
        int apagarX = roboX + i;
        int apagarY = roboY + j;

        if (apagarX >= 0 && apagarX < MAP_SIZE && apagarY >= 0 && apagarY < MAP_SIZE) {
          matrizMapa[apagarX][apagarY] = 0;
        }
      }
    }
    xSemaphoreGive(xMutexMapa);
  }
}

void processarComandoJoystick(int velocidade, int angulo) {
  if (estadoAtual == ESTADO_EMERGENCY) {
    motores.stop();
    velE_global = 0;
    velD_global = 0;
    return;
  }

  if (estadoAtual == ESTADO_IDLE && velocidade > 0) {
    mudarEstado(ESTADO_MANUAL);
  }

  int vE = 0;
  int vD = 0;

  if ((angulo >= 90) && (angulo <= 180)) {
    vE = velocidade * (135 - angulo) / 45;
    vD = velocidade;
  } else if ((angulo >= 0) && (angulo < 90)) {
    vE = velocidade;
    vD = velocidade * (angulo - 45) / 45;
  } else if ((angulo > 180) && (angulo <= 270)) {
    vE = -1 * velocidade;
    vD = -1 * velocidade * (angulo - 225) / 45;
  } else if (angulo > 270) {
    vE = -1 * velocidade * (315 - angulo) / 45;
    vD = -1 * velocidade;
  }

  velE_global = vE;
  velD_global = vD;

  // --- FILTRO ANTI-SPAM (Evita a fragmentação de memória e queda do Wi-Fi) ---
  static bool estavaAndando = false;
  static uint32_t tempoUltimoLog = 0;

  if (vE == 0 && vD == 0) {
    motores.stop();
    if (estavaAndando) {
      // Só envia log se passou 250ms desde a última mensagem
      if (millis() - tempoUltimoLog > 1000) {
        logEvento("A05", "Motores parados");
        tempoUltimoLog = millis();
      }
      estavaAndando = false;
    }
  } else {
    motores.turn(vE, vD);
    if (!estavaAndando) {
      // Só envia log se passou 250ms desde a última mensagem
      if (millis() - tempoUltimoLog > 1000) {
        logEvento("A06", "Movimento iniciado");
        tempoUltimoLog = millis();
      }
      estavaAndando = true;
    }
  }
}
